#include <gtk/gtk.h>

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

// ============================================================
//  main() — only creates objects and wires MVC together.
//  All event handling lives in the Controllers.
// ============================================================

int main(int argc, char* argv[]) {
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
    // BackupController wires all backup-related actions and acts as observer
    BackupController backupController(
        &backupModel, &mainView,
        &backupManager, &copyStrategy,
        &settingsModel
    );

    // SettingsController wires the Settings button and opens SettingsDialog
    SettingsController settingsController(&settingsModel, &mainView);

    // --- Start the application ---
    mainView.show();
    gtk_main();

    return 0;
}

