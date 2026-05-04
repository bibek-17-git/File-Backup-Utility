#include <gtk/gtk.h>
#include "model/BackupModel.cpp"
#include "model/SettingsModel.cpp"
#include "view/MainView.cpp"
#include "controller/BackupController.cpp"
#include "controller/SettingsController.cpp"
#include "strategies/SimpleCopyStrategy.h"
#include "core/BackupManager.h"

// Backup Observer class
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

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    
    // Create Models
    BackupModel backupModel;
    SettingsModel settingsModel;
    
    // Set default settings
    settingsModel.setDestination(".");
    settingsModel.setAutoBackup(false);
    settingsModel.setInterval(300);
    settingsModel.setIncludeSubfolders(true);
    settingsModel.setIncludeHidden(false);
    
    // Create View
    MainView mainView;
    
    // Create Strategy & BackupManager
    SimpleCopyStrategy copyStrategy;
    BackupManager backupManager(&copyStrategy);
    
    // Connect Model to View (Observer)
    backupModel.addObserver([&mainView, &backupModel]() {
        mainView.updateFileList(backupModel.getItems());
        if (!backupModel.isBackupRunning() && backupModel.getItemCount() > 0) {
            mainView.updateStatus(std::to_string(backupModel.getItemCount()) + " items ready", 0.0);
        }
    });
    
    // Setup View Callbacks
    mainView.onFileSelected([&backupModel](const std::vector<std::string>& paths, bool recursive) {
        for (const auto& path : paths) {
            backupModel.addItem(path);
        }
        (void)recursive; // unused parameter
    });
    
    mainView.onRemoveSelected([&backupModel, &mainView]() {
        std::string selected = mainView.getSelectedItem();
        if (!selected.empty()) {
            backupModel.removeItem(selected);
            mainView.updateStatus("Removed: " + selected, 0.0);
        } else {
            mainView.showNotification("Notice", "Please select an item to remove");
        }
    });
    
    mainView.onClearAll([&backupModel, &mainView]() {
        backupModel.clearItems();
        mainView.updateStatus("All items cleared", 0.0);
    });
    
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
        BackupSettings s = settingsModel.getSettings();
        std::string msg = "Current Settings:\nDestination: " + s.destination +
                          "\nAuto Backup: " + (s.autoBackup ? "Yes" : "No") +
                          "\nInterval: " + std::to_string(s.interval) + " seconds";
        mainView.showNotification("Settings", msg);
    });
    
    mainView.onViewLog([&mainView]() {
        mainView.showNotification("Log", "Backup logs are saved in the backup directory");
    });
    
    // Setup Backup Observer
    BackupObserver observer;
    observer.view = &mainView;
    observer.model = &backupModel;
    backupManager.setObserver(&observer);
    
    // Show the application
    mainView.show();
    
    gtk_main();
    return 0;
}
