#pragma once
#include "../strategies/SimpleCopyStrategy.h"
#include <vector>
#include <string>
#include <ctime>
#include <fstream>
#include <sys/stat.h>

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(p) _mkdir(p)
#else
    #define MKDIR(p) mkdir(p, 0755)
#endif

class IBackupObserver {
public:
    virtual ~IBackupObserver() = default;
    virtual void onProgress(int current, int total, const std::string& filename) = 0;
    virtual void onComplete(int success, int total) = 0;
    virtual void onError(const std::string& message) = 0;
};

class BackupManager {
private:
    IBackupStrategy* m_strategy;
    IBackupObserver* m_observer = nullptr;
    bool m_running = false;

    std::string joinPath(const std::string& a, const std::string& b) {
        if (a.empty()) return b;
        if (a.back() == '/' || a.back() == '\\') return a + b;
#ifdef _WIN32
        return a + "\\" + b;
#else
        return a + "/" + b;
#endif
    }

    std::string baseName(const std::string& path) {
        size_t pos = path.find_last_of("/\\");
        return (pos == std::string::npos) ? path : path.substr(pos + 1);
    }

    std::string timestamp() {
        time_t now = time(nullptr);
        char buf[32];
        strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", localtime(&now));
        return buf;
    }

    void createDir(const std::string& path) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) {
            MKDIR(path.c_str());
        }
    }

    // File নাকি Folder সেটা detect করে
    bool isDirectory(const std::string& path) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) return false;
        return S_ISDIR(st.st_mode);
    }

    // Folder এর ভেতরে কতটা file আছে count করে (progress এর জন্য)
    int countFiles(const std::string& path) {
        if (!isDirectory(path)) return 1;
        int count = 0;

#ifdef _WIN32
        std::string searchPath = path + "\\*";
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
        if (hFind == INVALID_HANDLE_VALUE) return 0;
        do {
            std::string name = findData.cFileName;
            if (name == "." || name == "..") continue;
            std::string fullPath = path + "\\" + name;
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                count += countFiles(fullPath);
            else
                count++;
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
#endif
        return count;
    }

public:
    BackupManager(IBackupStrategy* strategy) : m_strategy(strategy) {}
    void setObserver(IBackupObserver* observer) { m_observer = observer; }
    bool isRunning() const { return m_running; }

    bool runBackup(const std::vector<std::string>& items, const std::string& destination) {
        if (m_running || items.empty()) return false;
        m_running = true;

        std::string backupDir = joinPath(destination, "Backup_" + timestamp());
        createDir(backupDir);

        std::string logPath = joinPath(backupDir, "backup_log.txt");
        std::ofstream log(logPath, std::ios::app);

        time_t now = time(nullptr);
        char timeBuf[64];
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        log << "Backup started: " << timeBuf << "\n";

        int total   = items.size();
        int success = 0;

        for (int i = 0; i < total; ++i) {
            std::string name = baseName(items[i]);
            std::string dest = joinPath(backupDir, name);

            if (m_observer) m_observer->onProgress(i + 1, total, name);

            if (isDirectory(items[i])) {
                // ── Folder → recursive copy ──
                log << "[FOLDER] " << items[i] << "\n";
                if (m_strategy->copyFolder(items[i], dest)) {
                    success++;
                    log << "[OK] Folder backed up: " << name << "\n";
                } else {
                    log << "[ERR] Folder failed: " << name << "\n";
                }
            } else {
                // ── Single file copy ──
                if (m_strategy->copyFile(items[i], dest)) {
                    success++;
                    log << "[OK] " << items[i] << "\n";
                } else {
                    log << "[ERR] " << items[i] << "\n";
                }
            }
        }

        log << "Backup complete: " << success << "/" << total << " items\n";
        log.close();

        if (m_observer) m_observer->onComplete(success, total);
        m_running = false;
        return success == total;
    }
};
