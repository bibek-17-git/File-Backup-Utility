#pragma once
#include "ISettingsModel.h"
#include <vector>

class SettingsModel : public ISettingsModel {
private:
    BackupSettings m_settings;
    std::vector<ObserverCallback> m_observers;

public:
    BackupSettings getSettings() const override;
    void setDestination(const std::string& dest) override;
    void setAutoBackup(bool enabled) override;
    void setInterval(int seconds) override;
    void setIncludeSubfolders(bool include) override;
    void setIncludeHidden(bool include) override;
    void addObserver(ObserverCallback callback) override;
};
