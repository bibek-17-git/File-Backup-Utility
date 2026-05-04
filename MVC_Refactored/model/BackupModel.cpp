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
    void addItem(const std::string& path) override {
        if (m_itemSet.find(path) == m_itemSet.end()) {
            m_items.push_back(path);
            m_itemSet.insert(path);
            for (auto& cb : m_observers) cb();
        }
    }
    
    void removeItem(const std::string& path) override {
        auto it = std::find(m_items.begin(), m_items.end(), path);
        if (it != m_items.end()) {
            m_items.erase(it);
            m_itemSet.erase(path);
            for (auto& cb : m_observers) cb();
        }
    }
    
    void clearItems() override {
        m_items.clear();
        m_itemSet.clear();
        for (auto& cb : m_observers) cb();
    }
    
    std::vector<std::string> getItems() const override { return m_items; }
    int getItemCount() const override { return m_items.size(); }
    
    bool isBackupRunning() const override { return m_backupRunning; }
    void setBackupRunning(bool running) override { 
        m_backupRunning = running;
        if (!running) m_currentProgress = 0;
        for (auto& cb : m_observers) cb();
    }
    
    int getCurrentProgress() const override { return m_currentProgress; }
    int getTotalItems() const override { return m_totalItems; }
    void updateProgress(int current, int total, const std::string& filename) override {
        m_currentProgress = current;
        m_totalItems = total;
        m_currentFilename = filename;
        for (auto& cb : m_observers) cb();
    }
    
    std::string getCurrentFilename() const override { return m_currentFilename; }
    void resetResults() override { for (auto& cb : m_observers) cb(); }
    
    void addObserver(ObserverCallback callback) override {
        m_observers.push_back(callback);
    }
};
