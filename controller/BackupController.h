#pragma once
#include "../model/IBackupModel.h"
#include "../model/ISettingsModel.h"
#include "../view/IMainView.h"
#include "../core/BackupManager.h"
#include "../strategies/SimpleCopyStrategy.h"

class BackupController : public IBackupObserver {
private:
    IBackupModel*    m_model;
    IMainView*       m_view;
    BackupManager*   m_backupManager;
    IBackupStrategy* m_strategy;
    ISettingsModel*  m_settingsModel;

    void setupCallbacks();
    void setupModelObservers();
    void syncModelToView();

    void handleFileSelection(const std::vector<std::string>& paths, bool recursive);
    void handleRemoveSelected();
    void handleClearAll();
    void handleStartBackup();
    void handleViewLog();

public:
    BackupController(IBackupModel* model, IMainView* view,
                     BackupManager* backupMgr, IBackupStrategy* strategy,
                     ISettingsModel* settingsModel);

    // IBackupObserver
    void onProgress(int current, int total, const std::string& filename) override;
    void onComplete(int success, int total) override;
    void onError(const std::string& message) override;
};
