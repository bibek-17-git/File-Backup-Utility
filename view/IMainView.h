#pragma once
#include <string>
#include <vector>
#include <functional>

class IMainView {
public:
    virtual ~IMainView() = default;
    
    virtual void show() = 0;
    virtual void updateStatus(const std::string& message, double progress) = 0;
    virtual void updateFileList(const std::vector<std::string>& files) = 0;
    virtual void showNotification(const std::string& title, const std::string& message) = 0;
    virtual void showError(const std::string& error) = 0;
    virtual void setBackupButtonEnabled(bool enabled) = 0;
    virtual std::string getSelectedItem() = 0;
    
    using FileSelectedCallback = std::function<void(const std::vector<std::string>&, bool)>;
    using RemoveSelectedCallback = std::function<void()>;
    using ClearAllCallback = std::function<void()>;
    using StartBackupCallback = std::function<void()>;
    using OpenSettingsCallback = std::function<void()>;
    using ViewLogCallback = std::function<void()>;
    
    virtual void onFileSelected(FileSelectedCallback callback) = 0;
    virtual void onRemoveSelected(RemoveSelectedCallback callback) = 0;
    virtual void onClearAll(ClearAllCallback callback) = 0;
    virtual void onStartBackup(StartBackupCallback callback) = 0;
    virtual void onOpenSettings(OpenSettingsCallback callback) = 0;
    virtual void onViewLog(ViewLogCallback callback) = 0;
};
