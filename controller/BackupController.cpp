#include "BackupController.h"
#include <string>

BackupController::BackupController(IBackupModel* model, IMainView* view,
                                   BackupManager* backupMgr, IBackupStrategy* strategy,
                                   ISettingsModel* settingsModel)
    : m_model(model), m_view(view), m_backupManager(backupMgr),
      m_strategy(strategy), m_settingsModel(settingsModel)
{
    setupCallbacks();
    setupModelObservers();
    m_backupManager->setObserver(this);
}

void BackupController::setupCallbacks() {
    m_view->onFileSelected([this](const std::vector<std::string>& paths, bool recursive) {
        handleFileSelection(paths, recursive);
    });
    m_view->onRemoveSelected([this]() { handleRemoveSelected(); });
    m_view->onClearAll([this]()       { handleClearAll();       });
    m_view->onStartBackup([this]()    { handleStartBackup();    });
    m_view->onViewLog([this]()        { handleViewLog();        });
    // onOpenSettings is handled by SettingsController
}

void BackupController::setupModelObservers() {
    m_model->addObserver([this]() { syncModelToView(); });
}

void BackupController::syncModelToView() {
    m_view->updateFileList(m_model->getItems());

    if (m_model->isBackupRunning()) {
        int current = m_model->getCurrentProgress();
        int total   = m_model->getTotalItems();
        if (total > 0) {
            m_view->updateStatus("Backing up: " + m_model->getCurrentFilename(),
                                  static_cast<double>(current) / total);
        }
    } else {
        int count = m_model->getItemCount();
        if (count > 0)
            m_view->updateStatus(std::to_string(count) + " items ready", 0.0);
        else
            m_view->updateStatus("✓ Ready", 0.0);
    }
}

void BackupController::handleFileSelection(const std::vector<std::string>& paths, bool /*recursive*/) {
    for (const auto& path : paths)
        m_model->addItem(path);
}

void BackupController::handleRemoveSelected() {
    std::string selected = m_view->getSelectedItem();
    if (!selected.empty()) {
        m_model->removeItem(selected);
        m_view->updateStatus("Removed: " + selected, 0.0);
    } else {
        m_view->showNotification("Notice", "Please select an item to remove");
    }
}

void BackupController::handleClearAll() {
    m_model->clearItems();
    m_view->updateStatus("All items cleared", 0.0);
}

void BackupController::handleStartBackup() {
    if (m_model->isBackupRunning()) {
        m_view->showError("Backup already running");
        return;
    }
    if (m_model->getItemCount() == 0) {
        m_view->showError("No items selected");
        return;
    }

    std::string destination = m_settingsModel->getSettings().destination;
    if (destination.empty()) {
        m_view->showError("No backup destination configured");
        return;
    }

    m_view->setBackupButtonEnabled(false);
    m_model->resetResults();
    m_model->setBackupRunning(true);
    m_backupManager->runBackup(m_model->getItems(), destination);
}

void BackupController::handleViewLog() {
    m_view->showNotification("Log", "Backup logs are saved in the backup destination folder.");
}

// ── IBackupObserver ────────────────────────────────────────────────────────

void BackupController::onProgress(int current, int total, const std::string& filename) {
    m_model->updateProgress(current, total, filename);
}

void BackupController::onComplete(int success, int total) {
    m_model->setBackupRunning(false);
    m_view->setBackupButtonEnabled(true);

    std::string msg = "Backup complete: " + std::to_string(success) +
                      "/" + std::to_string(total) + " files backed up";
    m_view->updateStatus(msg, 1.0);
    m_view->showNotification("Backup Complete", msg);
}

void BackupController::onError(const std::string& message) {
    m_model->setBackupRunning(false);
    m_view->setBackupButtonEnabled(true);
    m_view->showError(message);
}

