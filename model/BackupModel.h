#pragma once
#include "IBackupModel.h"
#include <unordered_set>
#include <algorithm>

class BackupModel : public IBackupModel {
private:
    std::vector<std::string> m_items;
    std::unordered_set<std::string> m_itemSet;
    bool m_backupRunning = false;
    int m_currentProgress = 0;
    int m_totalItems = 0;
    std::string m_currentFilename;
    std::vector<ObserverCallback> m_observers;

public:
    void addItem(const std::string& path) override;
    void removeItem(const std::string& path) override;
    void clearItems() override;
    std::vector<std::string> getItems() const override;
    int getItemCount() const override;
    bool isBackupRunning() const override;
    void setBackupRunning(bool running) override;
    int getCurrentProgress() const override;
    int getTotalItems() const override;
    void updateProgress(int current, int total, const std::string& filename) override;
    std::string getCurrentFilename() const override;
    void resetResults() override;
    void addObserver(ObserverCallback callback) override;
};
