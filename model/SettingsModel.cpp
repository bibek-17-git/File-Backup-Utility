#include "SettingsModel.h"

BackupSettings SettingsModel::getSettings() const { return m_settings; }

void SettingsModel::setDestination(const std::string& dest) {
    m_settings.destination = dest;
    for (auto& cb : m_observers) cb();
}

void SettingsModel::setAutoBackup(bool enabled) {
    m_settings.autoBackup = enabled;
    for (auto& cb : m_observers) cb();
}

void SettingsModel::setInterval(int seconds) {
    m_settings.interval = seconds;
    for (auto& cb : m_observers) cb();
}

void SettingsModel::setIncludeSubfolders(bool include) {
    m_settings.includeSubfolders = include;
    for (auto& cb : m_observers) cb();
}

void SettingsModel::setIncludeHidden(bool include) {
    m_settings.includeHidden = include;
    for (auto& cb : m_observers) cb();
}

void SettingsModel::addObserver(ObserverCallback callback) {
    m_observers.push_back(callback);
}
