#include "../model/ISettingsModel.h"
#include "../view/IMainView.h"
#include <string>

class SettingsController {
private:
    ISettingsModel* m_model;
    IMainView* m_view;
    
public:
    SettingsController(ISettingsModel* model, IMainView* view)
        : m_model(model), m_view(view) {
        
        m_view->onOpenSettings([this]() {
            showSettingsDialog();
        });
    }
    
    void showSettingsDialog() {
        BackupSettings current = m_model->getSettings();
        
        // For now, show a simple dialog
        std::string message = "Current Settings:\n";
        message += "Destination: " + current.destination + "\n";
        message += "Auto Backup: " + std::string(current.autoBackup ? "Yes" : "No") + "\n";
        message += "Interval: " + std::to_string(current.interval) + " seconds\n";
        message += "Max Copies: " + std::to_string(current.maxCopies) + "\n";
        message += "Include Subfolders: " + std::string(current.includeSubfolders ? "Yes" : "No") + "\n";
        message += "Include Hidden: " + std::string(current.includeHidden ? "Yes" : "No");
        
        m_view->showNotification("Settings", message);
    }
    
    void saveSettings(const std::string& dest, bool autoBackup,
                      int interval, int maxCopies,
                      bool subfolders, bool hidden) {
        m_model->setDestination(dest);
        m_model->setAutoBackup(autoBackup);
        m_model->setInterval(interval);
        m_model->setIncludeSubfolders(subfolders);
        m_model->setIncludeHidden(hidden);
        
        m_view->showNotification("Settings", "Settings saved successfully");
    }
};
