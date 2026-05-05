#include "BackupModel.h"

void BackupModel::addItem(const std::string& path) {
    if (m_itemSet.find(path) == m_itemSet.end()) {
        m_items.push_back(path);
        m_itemSet.insert(path);
        for (auto& cb : m_observers) cb();
    }
}

void BackupModel::removeItem(const std::string& path) {
    auto it = std::find(m_items.begin(), m_items.end(), path);
    if (it != m_items.end()) {
        m_items.erase(it);
        m_itemSet.erase(path);
        for (auto& cb : m_observers) cb();
    }
}

void BackupModel::clearItems() {
    m_items.clear();
    m_itemSet.clear();
    for (auto& cb : m_observers) cb();
}

std::vector<std::string> BackupModel::getItems() const { return m_items; }
int BackupModel::getItemCount() const { return (int)m_items.size(); }

bool BackupModel::isBackupRunning() const { return m_backupRunning; }

void BackupModel::setBackupRunning(bool running) {
    m_backupRunning = running;
    if (!running) m_currentProgress = 0;
    for (auto& cb : m_observers) cb();
}

int BackupModel::getCurrentProgress() const { return m_currentProgress; }
int BackupModel::getTotalItems() const { return m_totalItems; }

void BackupModel::updateProgress(int current, int total, const std::string& filename) {
    m_currentProgress = current;
    m_totalItems = total;
    m_currentFilename = filename;
    for (auto& cb : m_observers) cb();
}

std::string BackupModel::getCurrentFilename() const { return m_currentFilename; }

void BackupModel::resetResults() {
    m_currentProgress = 0;
    m_totalItems = 0;
    m_currentFilename = "";
    for (auto& cb : m_observers) cb();
}

void BackupModel::addObserver(ObserverCallback callback) {
    m_observers.push_back(callback);
}
