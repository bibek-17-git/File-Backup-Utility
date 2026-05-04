#include <gtk/gtk.h>
#include "model/BackupModel.cpp"
#include "model/SettingsModel.cpp"
#include "view/MainView.cpp"
#include "view/SettingsDialog.h"
#include "strategies/SimpleCopyStrategy.h"
#include "core/BackupManager.h"

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    
    // Create Models
    BackupModel backupModel;
    SettingsModel settingsModel;
    
    // Set default settings
    settingsModel.setDestination("C:\\Backups");
    settingsModel.setAutoBackup(false);
    settingsModel.setInterval(300);
    settingsModel.setIncludeSubfolders(true);
    settingsModel.setIncludeHidden(false);
    
    // Create View
    MainView mainView;
    
    // Create Strategy & BackupManager
    SimpleCopyStrategy copyStrategy;
    BackupManager backupManager(&copyStrategy);
    
    // Connect Model to View
    backupModel.addObserver([&mainView, &backupModel]() {
        mainView.updateFileList(backupModel.getItems());
        if (!backupModel.isBackupRunning() && backupModel.getItemCount() > 0) {
            mainView.updateStatus(std::to_string(backupModel.getItemCount()) + " items ready", 0.0);
        }
    });
    
    // Setup Controller logic directly in main (simplified)
    mainView.onFileSelected([&backupModel, &settingsModel](const std::vector<std::string>& paths, bool recursive) {
        for (const auto& path : paths) {
            backupModel.addItem(path);
        }
        mainView.updateStatus(std::to_string(backupModel.getItemCount()) + " items added", 0.0);
    });
    
    mainView.onRemoveSelected([&backupModel, &mainView]() {
        std::string selected = mainView.getSelectedItem();
        if (!selected.empty()) {
            backupModel.removeItem(selected);
            mainView.updateStatus("Removed: " + selected, 0.0);
        }
    });
    
    mainView.onClearAll([&backupModel, &mainView]() {
        backupModel.clearItems();
        mainView.updateStatus("All items cleared", 0.0);
    });
    
    // Setup backup observer
    class BackupObserver : public IBackupObserver {
    public:
        MainView* view;
        IBackupModel* model;
        
        void onProgress(int current, int total, const std::string& filename) override {
            model->updateProgress(current, total, filename);
            view->updateStatus("Backing up: " + filename, (double)current / total);
        }
        
        void onComplete(int success, int total) override {
            model->setBackupRunning(false);
            view->setBackupButtonEnabled(true);
            view->updateStatus("Complete: " + std::to_string(success) + "/" + std::to_string(total), 1.0);
            view->showNotification("Backup Complete", std::to_string(success) + " files backed up");
        }
        
        void onError(const std::string& message) override {
            model->setBackupRunning(false);
            view->setBackupButtonEnabled(true);
            view->showError(message);
        }
    };
    
    BackupObserver observer;
    observer.view = &mainView;
    observer.model = &backupModel;
    backupManager.setObserver(&observer);
    
    mainView.onStartBackup([&backupModel, &backupManager, &settingsModel, &mainView]() {
        if (backupModel.isBackupRunning()) {
            mainView.showError("Backup already running");
            return;
        }
        if (backupModel.getItemCount() == 0) {
            mainView.showError("No items selected");
            return;
        }
        mainView.setBackupButtonEnabled(false);
        backupModel.setBackupRunning(true);
        backupManager.runBackup(backupModel.getItems(), settingsModel.getSettings().destination);
    });
    
    mainView.onOpenSettings([&settingsModel, &mainView]() {
        auto s = settingsModel.getSettings();
        SettingsDialog::show(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(mainView.show() == 0 ? nullptr : nullptr))),
            s.destination, s.autoBackup, s.interval, s.maxCopies,
            s.includeSubfolders, s.includeHidden,
            [&settingsModel](const std::string& d, bool a, int i, int m, bool sf, bool h) {
                settingsModel.setDestination(d);
