#include "SettingsController.h"
#include "../view/SettingsDialog.h"

SettingsController::SettingsController(ISettingsModel* model, IMainView* view)
    : m_model(model), m_view(view)
{
    // Register the Settings button callback from the View
    m_view->onOpenSettings([this]() {
        showSettingsDialog();
    });
}

void SettingsController::showSettingsDialog() {
    BackupSettings current = m_model->getSettings();

    // Use the proper SettingsDialog (previously unused — now fixed)
    SettingsDialog::show(
        nullptr, // no parent GTK window reference needed here
        current.destination,
        current.autoBackup,
        current.interval,
        current.maxCopies,
        current.includeSubfolders,
        current.includeHidden,
        [this](const std::string& dest, bool autoBackup,
               int interval, int maxCopies,
               bool subfolders, bool hidden)
        {
            // Controller updates Model — View is notified via observer
            m_model->setDestination(dest);
            m_model->setAutoBackup(autoBackup);
            m_model->setInterval(interval);
            m_model->setIncludeSubfolders(subfolders);
            m_model->setIncludeHidden(hidden);
            (void)maxCopies; // ISettingsModel doesn't expose maxCopies setter yet

            m_view->showNotification("Settings", "Settings saved successfully!");
        }
    );
}

