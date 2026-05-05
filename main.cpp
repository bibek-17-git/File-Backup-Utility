#include <gtk/gtk.h>

#ifdef _WIN32
#include <windows.h>

void EnsureConsole() {
    if (GetConsoleWindow() == nullptr) {
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        SetConsoleTitleA("Smart Backup - Debug Console");  // Fixed
        
        // Optional: Make console bigger for better visibility
        HWND console = GetConsoleWindow();
        MoveWindow(console, 100, 100, 800, 400, TRUE);
    }
}
#endif

// Model
#include "model/BackupModel.h"
#include "model/SettingsModel.h"

// View
#include "view/MainView.h"

// Controller
#include "controller/BackupController.h"
#include "controller/SettingsController.h"

// Core
#include "core/BackupManager.h"
#include "strategies/SimpleCopyStrategy.h"

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    EnsureConsole();
    #endif
    
    gtk_init(&argc, &argv);

    // --- Model layer ---
    BackupModel   backupModel;
    SettingsModel settingsModel;

    // Default settings
    settingsModel.setDestination(".");
    settingsModel.setAutoBackup(false);
    settingsModel.setInterval(300);
    settingsModel.setIncludeSubfolders(true);
    settingsModel.setIncludeHidden(false);

    // --- View layer ---
    MainView mainView;

    // --- Core / Strategy ---
    SimpleCopyStrategy copyStrategy;
    BackupManager      backupManager(&copyStrategy);

    // --- Controller layer ---
    BackupController backupController(
        &backupModel, &mainView,
        &backupManager, &copyStrategy,
        &settingsModel
    );

    SettingsController settingsController(&settingsModel, &mainView);

    // --- Start the application ---
    mainView.show();
    gtk_main();

    return 0;
}