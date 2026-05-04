#include "ISettingsModel.h"
#include <vector>

class SettingsModel : public ISettingsModel {
private:
    BackupSettings m_settings;
    std::vector<ObserverCallback> m_observers;
    
public:
    BackupSettings getSettings() const override { return m_settings; }
    
    void setDestination(const std::string& dest) override {
        m_settings.destination = dest;
        for (auto& cb : m_observers) cb();
    }
    
    void setAutoBackup(bool enabled) override {
        m_settings.autoBackup = enabled;
        for (auto& cb : m_observers) cb();
    }
    
    void setInterval(int seconds) override {
        m_settings.interval = seconds;
        for (auto& cb : m_observers) cb();
    }
    
    void setIncludeSubfolders(bool include) override {
        m_settings.includeSubfolders = include;
        for (auto& cb : m_observers) cb();
    }
    
    void setIncludeHidden(bool include) override {
        m_settings.includeHidden = include;
        for (auto& cb : m_observers) cb();
    }
    
    void addObserver(ObserverCallback callback) override {
        m_observers.push_back(callback);
    }
};
