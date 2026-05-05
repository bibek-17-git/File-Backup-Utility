#pragma once
#include "IMainView.h"
#include <gtk/gtk.h>
#include <vector>
#include <string>

class MainView : public IMainView {
private:
    GtkWidget* m_window;
    GtkWidget* m_progressBar;
    GtkWidget* m_statusLabel;
    GtkListStore* m_listStore;
    GtkWidget* m_treeView;
    GtkWidget* m_startButton;
    GtkTreeSelection* m_selection;

    FileSelectedCallback   m_onFileSelected;
    RemoveSelectedCallback m_onRemoveSelected;
    ClearAllCallback       m_onClearAll;
    StartBackupCallback    m_onStartBackup;
    OpenSettingsCallback   m_onOpenSettings;
    ViewLogCallback        m_onViewLog;

    std::string formatSize(long bytes);
    std::string lastModified(const std::string& path);
    std::vector<std::string> showFileChooserDialog();
    std::vector<std::string> showFolderDialog();
    void buildUI();

    // GTK static signal callbacks
    static void onAddFilesClicked(GtkWidget*, gpointer data);
    static void onAddFolderClicked(GtkWidget*, gpointer data);
    static void onRemoveSelectedClicked(GtkWidget*, gpointer data);
    static void onClearAllClicked(GtkWidget*, gpointer data);
    static void onStartBackupClicked(GtkWidget*, gpointer data);
    static void onSettingsClicked(GtkWidget*, gpointer data);
    static void onViewLogClicked(GtkWidget*, gpointer data);

public:
    MainView();

    void show() override;
    void updateStatus(const std::string& message, double progress) override;
    void updateFileList(const std::vector<std::string>& files) override;
    void showNotification(const std::string& title, const std::string& message) override;
    void showError(const std::string& error) override;
    void setBackupButtonEnabled(bool enabled) override;
    std::string getSelectedItem() override;

    void onFileSelected(FileSelectedCallback callback) override;
    void onRemoveSelected(RemoveSelectedCallback callback) override;
    void onClearAll(ClearAllCallback callback) override;
    void onStartBackup(StartBackupCallback callback) override;
    void onOpenSettings(OpenSettingsCallback callback) override;
    void onViewLog(ViewLogCallback callback) override;
};
