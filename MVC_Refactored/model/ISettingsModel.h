#pragma once
#include <string>
#include <functional>

struct BackupSettings {
    std::string destination = "C:\\Backups";
    bool autoBackup = false;
    int interval = 300;
    int maxCopies = 10;
    bool includeSubfolders = true;
    bool includeHidden = false;
    bool showNotifications = true;
};

class ISettingsModel {
public:
    virtual ~ISettingsModel() = default;
    virtual BackupSettings getSettings() const = 0;
    virtual void setDestination(const std::string& dest) = 0;
    virtual void setAutoBackup(bool enabled) = 0;
    virtual void setInterval(int seconds) = 0;
    virtual void setIncludeSubfolders(bool include) = 0;
    virtual void setIncludeHidden(bool include) = 0;
    using ObserverCallback = std::function<void()>;
    virtual void addObserver(ObserverCallback callback) = 0;
};
