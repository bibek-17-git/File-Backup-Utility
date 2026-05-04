#pragma once
#include <string>
#include <vector>
#include <functional>

class IBackupModel {
public:
    virtual ~IBackupModel() = default;
    
    virtual void addItem(const std::string& path) = 0;
    virtual void removeItem(const std::string& path) = 0;
    virtual void clearItems() = 0;
    virtual std::vector<std::string> getItems() const = 0;
    virtual int getItemCount() const = 0;
    
    virtual bool isBackupRunning() const = 0;
    virtual void setBackupRunning(bool running) = 0;
    
    virtual int getCurrentProgress() const = 0;
    virtual int getTotalItems() const = 0;
    virtual void updateProgress(int current, int total, const std::string& filename) = 0;
    virtual std::string getCurrentFilename() const = 0;
    
    virtual void resetResults() = 0;
    
    using ObserverCallback = std::function<void()>;
    virtual void addObserver(ObserverCallback callback) = 0;
};
