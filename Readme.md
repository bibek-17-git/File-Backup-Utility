# File Backup Utility — MVC Architecture Branch

> **Branch purpose:** This branch refactors the **File Backup Utility** to follow the **Model-View-Controller (MVC)** design pattern. The entire application — file list management, backup execution, settings handling, and progress reporting — is reorganised so that data, rendering, and event-handling each have a clearly separated home.

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Why MVC — and Why Here?](#why-mvc--and-why-here)
3. [Architecture at a Glance](#architecture-at-a-glance)
4. [Folder Structure](#folder-structure)
5. [The MVC Triad in Detail](#the-mvc-triad-in-detail)
   - [Model — `BackupModel` & `SettingsModel`](#model--backupmodel--settingsmodel)
   - [View — `MainView`](#view--mainview)
   - [Controller — `BackupController` & `SettingsController`](#controller--backupcontroller--settingscontroller)
6. [How the Pieces Connect — `main()` as the Orchestrator](#how-the-pieces-connect--main-as-the-orchestrator)
7. [The Full Backup Flow (Step by Step)](#the-full-backup-flow-step-by-step)
8. [Data Flow Diagram](#data-flow-diagram)
9. [Supporting Systems](#supporting-systems)
   - [BackupManager & IBackupObserver](#backupmanager--ibackupobserver)
   - [IBackupStrategy / SimpleCopyStrategy](#ibackupstrategy--simplecopystrategy)
   - [IBackupModel & ISettingsModel](#ibackupmodel--isettingsmodel)
   - [IMainView](#imainview)
   - [SettingsDialog](#settingsdialog)
10. [Class Responsibility Table](#class-responsibility-table)
11. [Key Design Decisions](#key-design-decisions)
12. [Building the Project](#building-the-project)

---

## Project Overview

**File Backup Utility** is a desktop file backup tool built in **C with GTK3**. Users can select individual files or folders, configure a backup destination, and trigger an immediate copy. The backup engine timestamps each run, writes a log, and reports live progress to the UI. The project supports both Linux and Windows (MSYS2/MinGW).

**Tech stack:** C++17 · GTK3 (`gtk+-3.0`) · POSIX filesystem API

---

## Why MVC — and Why Here?

Before this refactor, all logic — UI callbacks, file copying, settings persistence, progress tracking — was crammed into a single monolithic GTK source file (~450 lines). Any change to one concern risked breaking the others, and testing individual pieces was impossible without the full GUI.

MVC solves this by giving each concern its own home:

| Concern | MVC Role | Class(es) |
|---|---|---|
| What the application data looks like | **Model** | `BackupModel`, `SettingsModel` |
| How the data is displayed to the user | **View** | `MainView`, `SettingsDialog` |
| How UI events and backup results mutate the data | **Controller** | `BackupController`, `SettingsController` |

The backup execution engine (`BackupManager`) and copy algorithm (`SimpleCopyStrategy`) are factored out as separate domain objects — they know nothing about GTK and can be reused or tested independently.

---

## Architecture at a Glance

```
main()
  ├── BackupModel         ← M: file-list state + progress + observer list
  ├── SettingsModel       ← M: destination, interval, flags + observer list
  ├── MainView            ← V: GTK window, file list, status bar, buttons
  ├── SimpleCopyStrategy  ← Domain: binary file copy algorithm
  ├── BackupManager       ← Domain: orchestrates copy loop, writes log
  ├── BackupController    ← C: wires View callbacks → Model mutations + BackupManager
  └── SettingsController  ← C: wires Settings button → SettingsDialog → SettingsModel
```

---

## Folder Structure

```
MVC_Refactored/
├── .gitignore
├── main.cpp                        # Wires all objects together, calls gtk_main()
├── Makefile
│
├── model/
│   ├── IBackupModel.h              # Abstract interface for backup state
│   ├── BackupModel.cpp             # Concrete file-list + progress model
│   ├── ISettingsModel.h            # Abstract interface for settings + BackupSettings struct
│   └── SettingsModel.cpp           # Concrete settings model
│
├── view/
│   ├── IMainView.h                 # Abstract GTK view interface + callback typedefs
│   └── MainView.cpp                # Concrete GTK3 main window
│
├── controller/
│   ├── BackupController.cpp        # Handles all backup-related UI events
│   └── SettingsController.cpp      # Handles Settings button → dialog → model
│
├── strategies/
│   └── SimpleCopyStrategy.h        # IBackupStrategy + SimpleCopyStrategy (binary copy)
│
└── core/
    └── BackupManager.h             # Copy loop, log writing, IBackupObserver interface
```

---

## The MVC Triad in Detail

### Model — `BackupModel` & `SettingsModel`

**Files:** `model/IBackupModel.h` · `model/BackupModel.h` · `model/BackupModel.cpp`
`model/ISettingsModel.h` · `model/SettingsModel.h` · `model/SettingsModel.cpp`

The Model layer is the single source of truth for all application data. It holds no GTK handles, renders nothing, and has no knowledge of how data will be displayed.

#### `BackupModel`

```cpp
class BackupModel : public IBackupModel {
private:
    std::vector<std::string>            m_items;        // Ordered list of selected paths
    std::unordered_set<std::string>     m_itemSet;      // Fast duplicate guard
    bool                                m_backupRunning = false;
    int                                 m_currentProgress = 0;
    int                                 m_totalItems = 0;
    std::string                         m_currentFilename;
    std::vector<ObserverCallback>       m_observers;    // Notified on every mutation
};
```

**Key operations:**

- `addItem(path)` — adds a path only if not already present (duplicate-safe), then notifies all observers.
- `removeItem(path)` — removes from both vector and set, then notifies observers.
- `updateProgress(current, total, filename)` — called by the Controller on each `IBackupObserver::onProgress` tick; updates progress fields and notifies observers.
- `setBackupRunning(bool)` — resets `m_currentProgress` to 0 on `false`; notifies observers.

**Observer pattern:**

```cpp
void BackupModel::addItem(const std::string& path) {
    if (m_itemSet.find(path) == m_itemSet.end()) {
        m_items.push_back(path);
        m_itemSet.insert(path);
        for (auto& cb : m_observers) cb();   // View re-reads model immediately
    }
}
```

Every public mutation ends with a fan-out to all registered callbacks. The View registers a lambda that calls `syncModelToView()` on every notification, so the GTK file list and status bar are always in sync with the model without any polling.

#### `SettingsModel`

Wraps a plain `BackupSettings` struct:

```cpp
struct BackupSettings {
    std::string destination    = "C:\\Backups";
    bool        autoBackup     = false;
    int         interval       = 300;      // seconds
    int         maxCopies      = 10;
    bool        includeSubfolders = true;
    bool        includeHidden  = false;
    bool        showNotifications = true;
};
```

Individual setters (`setDestination`, `setAutoBackup`, `setInterval`, …) each notify observers, allowing the View to react if it ever displays current settings.

---

### View — `MainView`

**Files:** `view/IMainView.h` · `view/MainView.h` · `view/MainView.cpp` · `view/SettingsDialog.h`

The View is responsible only for GTK widget creation, layout, and rendering. It never modifies application state directly — it exposes **callback slots** that the Controller fills in.

```cpp
class IMainView {
public:
    // Display methods (called by Controller / BackupManager)
    virtual void show()                                                   = 0;
    virtual void updateStatus(const std::string& msg, double progress)    = 0;
    virtual void updateFileList(const std::vector<std::string>& files)    = 0;
    virtual void showNotification(const std::string& title,
                                  const std::string& message)             = 0;
    virtual void showError(const std::string& error)                      = 0;
    virtual void setBackupButtonEnabled(bool enabled)                     = 0;
    virtual std::string getSelectedItem()                                 = 0;

    // Callback registration (called once by Controllers during wiring)
    virtual void onFileSelected(FileSelectedCallback callback)    = 0;
    virtual void onRemoveSelected(RemoveSelectedCallback callback) = 0;
    virtual void onClearAll(ClearAllCallback callback)            = 0;
    virtual void onStartBackup(StartBackupCallback callback)      = 0;
    virtual void onOpenSettings(OpenSettingsCallback callback)    = 0;
    virtual void onViewLog(ViewLogCallback callback)              = 0;
};
```

**What `MainView` renders:**

- GTK3 window with a `GtkListBox` showing the selected file/folder paths
- A `GtkProgressBar` + status label updated by `updateStatus()`
- Toolbar buttons: **Add Files**, **Add Folder**, **Remove**, **Clear All**, **Start Backup**, **Settings**, **View Log**
- Error and notification dialogs via `showError()` and `showNotification()`

The View fires the registered lambdas when GTK signals arrive (e.g. button `clicked`, file-chooser `response`). It never decides *what to do* with those events — that is entirely the Controller's job.

---

### Controller — `BackupController` & `SettingsController`

**Files:** `controller/BackupController.h / .cpp` · `controller/SettingsController.h / .cpp`

The Controllers are the only components that write to the Models and the only components that decide when to play with the `BackupManager`.

#### `BackupController`

```cpp
class BackupController : public IBackupObserver {
private:
    IBackupModel*    m_model;
    IMainView*       m_view;
    BackupManager*   m_backupManager;
    IBackupStrategy* m_strategy;
    ISettingsModel*  m_settingsModel;

    void setupCallbacks();        // Registers View callback lambdas
    void setupModelObservers();   // Registers model → view sync observer
    void syncModelToView();       // Pushes current model state to view
    ...
};
```

**`setupCallbacks` — event wiring:**

```
View event                   Controller handler
──────────────────────────   ──────────────────────────────────────────────
onFileSelected(paths, rec)   → model->addItem(path) for each path
onRemoveSelected()           → model->removeItem(view->getSelectedItem())
onClearAll()                 → model->clearItems()
onStartBackup()              → validate → model->setBackupRunning(true)
                               → backupManager->runBackup(items, dest)
onViewLog()                  → view->showNotification(log location message)
```

**`IBackupObserver` callbacks (called by `BackupManager` during copy loop):**

```
onProgress(current, total, filename)  → model->updateProgress(...)
onComplete(success, total)            → model->setBackupRunning(false)
                                        → view->setBackupButtonEnabled(true)
                                        → view->showNotification(summary)
onError(message)                      → model->setBackupRunning(false)
                                        → view->showError(message)
```

The key point: the Controller acts as the bridge between `BackupManager` (domain) and `BackupModel` (state). The View is never touched directly inside the copy loop — it re-renders automatically when the Model notifies its observers.

#### `SettingsController`

```cpp
class SettingsController {
    SettingsController(ISettingsModel* model, IMainView* view);
    void showSettingsDialog();
};
```

Registers a single `onOpenSettings` callback. When the Settings button is clicked, it reads the current `BackupSettings` from the model, passes them to `SettingsDialog::show()`, and applies the user's confirmed changes back to the model via individual setters. The View is notified through the model's observer mechanism.

---

## How the Pieces Connect — `main()` as the Orchestrator

`main()` creates all objects on the stack, wires them together, and calls `gtk_main()`. It contains **no application logic** — just construction and dependency injection.

```cpp
int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);

    // Model layer
    BackupModel   backupModel;
    SettingsModel settingsModel;
    settingsModel.setDestination(".");
    settingsModel.setAutoBackup(false);
    settingsModel.setInterval(300);
    settingsModel.setIncludeSubfolders(true);
    settingsModel.setIncludeHidden(false);

    // View layer
    MainView mainView;

    // Domain layer
    SimpleCopyStrategy copyStrategy;
    BackupManager      backupManager(&copyStrategy);

    // Controller layer — wires everything
    BackupController  backupController(&backupModel, &mainView,
                                       &backupManager, &copyStrategy,
                                       &settingsModel);
    SettingsController settingsController(&settingsModel, &mainView);

    mainView.show();
    gtk_main();
    return 0;
}
```

---

## The Full Backup Flow (Step by Step)

```
① User clicks "Add Files" in MainView
    └─ GTK file-chooser dialog opens
    └─ onFileSelected lambda fires with chosen paths
    └─ BackupController::handleFileSelection()
         └─ model->addItem(path) for each path
              └─ Observer fires → syncModelToView() → view->updateFileList()

② User clicks "Start Backup"
    └─ onStartBackup lambda fires
    └─ BackupController::handleStartBackup()
         ├─ Guard: model->isBackupRunning()? → showError
         ├─ Guard: model->getItemCount() == 0? → showError
         ├─ Guard: settingsModel->getSettings().destination empty? → showError
         ├─ view->setBackupButtonEnabled(false)
         ├─ model->resetResults() + model->setBackupRunning(true)
         └─ backupManager->runBackup(items, destination)
              └─ Creates timestamped backup folder
              └─ For each file:
                   └─ strategy->copyFile(src, dest)
                   └─ observer->onProgress(i+1, total, filename)
                        └─ BackupController::onProgress()
                             └─ model->updateProgress(...)
                                  └─ Observer fires → syncModelToView()
                                       └─ view->updateStatus("Backing up: …", ratio)

③ BackupManager finishes
    └─ observer->onComplete(success, total)
         └─ BackupController::onComplete()
              ├─ model->setBackupRunning(false)
              ├─ view->setBackupButtonEnabled(true)
              ├─ view->updateStatus("Backup complete: X/Y files", 1.0)
              └─ view->showNotification("Backup Complete", summary)
```

---

## Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                          main()                             │
│  Creates and wires: Model · View · Manager · Controllers    │
└────────────┬──────────────────────────┬─────────────────────┘
             │                          │
    ┌────────▼────────┐       ┌─────────▼───────────┐
    │  BackupModel    │       │   SettingsModel      │
    │                 │       │                      │
    │ · m_items       │       │ · BackupSettings     │
    │ · progress      │       │   (destination,      │
    │ · observers     │       │    flags, interval)  │
    └────────┬────────┘       └──────────┬───────────┘
             │ notifies                  │ notifies
             ▼                           ▼
    ┌─────────────────────────────────────────────────┐
    │                   MainView                      │
    │  Reads model state via syncModelToView()        │
    │  Renders: file list · progress bar · status     │
    └───────────┬─────────────────────────────────────┘
                │ fires callbacks
                ▼
    ┌───────────────────────┐    ┌────────────────────────┐
    │   BackupController    │    │  SettingsController    │
    │                       │    │                        │
    │ · writes BackupModel  │    │ · opens SettingsDialog │
    │ · calls BackupManager │    │ · writes SettingsModel │
    │ · IBackupObserver     │    │                        │
    └───────────┬───────────┘    └────────────────────────┘
                │ calls
                ▼
    ┌───────────────────────┐
    │    BackupManager      │
    │                       │
    │ · creates backup dir  │
    │ · calls strategy per  │
    │   file                │
    │ · writes log          │
    │ · calls observer      │
    └───────────┬───────────┘
                │ uses
                ▼
    ┌───────────────────────┐
    │  SimpleCopyStrategy   │
    │  (IBackupStrategy)    │
    │  · binary file copy   │
    └───────────────────────┘

  GTK Events ──────────────────────────► BackupController / SettingsController
  IBackupObserver callbacks ───────────► BackupController → BackupModel
  Model observer notifications ────────► MainView (via syncModelToView lambda)
```

---

## Supporting Systems

### BackupManager & IBackupObserver

`BackupManager` is a pure domain object — it has no GTK dependency and no knowledge of MVC. It accepts an `IBackupStrategy*` at construction, an `IBackupObserver*` via `setObserver()`, and runs the copy loop synchronously inside `runBackup()`.

```cpp
bool BackupManager::runBackup(const std::vector<std::string>& items,
                               const std::string& destination);
```

Each iteration calls `strategy->copyFile(src, dest)`, then `observer->onProgress(i+1, total, filename)`. On completion it calls `observer->onComplete(success, total)`. If the observer is nullptr, the loop still runs safely — the observer calls are guarded.

`IBackupObserver` is a pure virtual interface with three methods: `onProgress`, `onComplete`, and `onError`. `BackupController` inherits it, making the Controller the natural recipient of all copy-loop events.

### IBackupStrategy / SimpleCopyStrategy

`IBackupStrategy` defines one method:

```cpp
virtual bool copyFile(const std::string& src, const std::string& dest) = 0;
```

`SimpleCopyStrategy` implements it with a binary stream copy (`ifstream`/`ofstream` with `rdbuf()`). Injecting the interface into `BackupManager` (and supplying `SimpleCopyStrategy` from `main.cpp`) means the copy algorithm is swappable without touching the Controller or Model — a direct application of the Dependency Inversion Principle. A future `CompressedCopyStrategy` or `EncryptedCopyStrategy` would drop in with zero changes elsewhere.

### IBackupModel & ISettingsModel

Both model classes expose only abstract interfaces to the Controllers and View. This means:

- `BackupController` holds `IBackupModel*` — it never depends on a concrete class.
- `SettingsController` holds `ISettingsModel*` — same guarantee.
- Either model can be swapped for a test double or a persistent-storage variant without any change to controller or view code.

### IMainView

`IMainView` abstracts the GTK window behind a pure C++ interface. The Controllers only ever call `IMainView*` methods. This separation means:

- The Controllers are fully unit-testable with a mock `IMainView`.
- A future Qt or ncurses front-end can implement `IMainView` and plug in with no controller changes.

### SettingsDialog

`SettingsDialog` is a utility class with a single static method:

```cpp
static void show(
    GtkWindow* parent,
    const std::string& currentDest, bool autoBackup, int interval,
    int maxCopies, bool subfolders, bool hidden,
    SettingsCallback onConfirm
);
```

It builds and runs a modal GTK dialog pre-populated with the current settings. If the user confirms, it calls `onConfirm` with the updated values. The `SettingsController` passes a lambda that writes each value back into `ISettingsModel`. The dialog owns no state of its own — it is entirely driven by the controller.

---

## Class Responsibility Table

| Class | Layer | Creates UI | Reads Model | Writes Model | Runs Backup | Renders |
|---|---|---|---|---|---|---|
| `BackupModel` | Model | ✗ | ✓ (self) | ✓ | ✗ | ✗ |
| `SettingsModel` | Model | ✗ | ✓ (self) | ✓ | ✗ | ✗ |
| `MainView` | View | ✓ | ✗ (push-only) | ✗ | ✗ | ✓ |
| `SettingsDialog` | View | ✓ | ✗ | ✗ | ✗ | ✓ |
| `BackupController` | Controller | ✗ | ✓ (via model) | ✓ | ✓ (via mgr) | ✗ |
| `SettingsController` | Controller | ✗ | ✓ (via model) | ✓ | ✗ | ✗ |
| `BackupManager` | Domain | ✗ | ✗ | ✗ | ✓ | ✗ |
| `SimpleCopyStrategy` | Domain | ✗ | ✗ | ✗ | ✓ (per file) | ✗ |
| `main()` | Orchestrator | ✗ | ✗ | ✗ | ✗ | ✗ |

---

## Key Design Decisions

**1. Two Models instead of one.**
Backup state (file list, progress) and configuration (destination, flags) have completely different lifetimes and mutation patterns. Keeping them in separate Models (`BackupModel` and `SettingsModel`) respects the Single Responsibility Principle and makes each independently testable.

**2. Observer pattern for Model → View sync.**
Rather than having the Controller push data to the View after every operation, the View registers a single observer lambda on the Model. This means every model mutation — wherever it originates — automatically triggers a re-render. There is no risk of a code path that forgets to update the UI.

**3. `BackupController` implements `IBackupObserver`.**
The Controller is the natural place to translate copy-loop events (`onProgress`, `onComplete`, `onError`) into model mutations and view notifications. Inheriting the interface avoids a separate adapter class and keeps all backup-related wiring in one file.

**4. `IBackupStrategy` injected into `BackupManager`.**
Copy algorithm, execution loop, and UI are three different axes of change. By injecting `IBackupStrategy`, adding a new copy mode (compressed, encrypted, delta) requires only a new strategy class — the Manager, Controller, Model, and View are untouched.

**5. `main()` contains zero application logic.**
All `main()` does is allocate objects and inject dependencies. This makes the wiring explicit and readable, and ensures that no important behavior is hidden in global initialisation.

**6. No threading in this branch.**
`BackupManager::runBackup()` is synchronous. The GTK event loop is blocked during a copy. This is a known simplification — a production version would run the copy loop on a worker thread and use `g_idle_add()` to push observer callbacks back onto the GTK thread. The MVC structure supports this upgrade with no architectural changes: only `BackupManager` and the GTK dispatch glue would change.

---

## Building the Project

**Prerequisites:** `g++` (C++17), GTK3 development headers (`libgtk-3-dev` on Debian/Ubuntu or equivalent).

```bash
# Clone this branch
git clone --branch mvc-implementation https://github.com/your-repo/file-backup-utility.git
cd file-backup-utility/MVC_Refactored

# Build
make

# Run
./smart_backup_mvc

# Clean build artifacts
make clean

# Run with memory check
make valgrind
```

The `Makefile` compiles all `.cpp` files under `model/`, `view/`, and `controller/` plus `main.cpp`, and links against GTK3 via `pkg-config`. Include paths are set to the project root so headers are always referenced from the root, e.g. `#include "model/IBackupModel.h"`.

**Platform notes:**

| Platform | Extra flags applied automatically |
|---|---|
| Windows (MSYS2/MinGW) | `-DWIN32 -static-libgcc -static-libstdc++` |
| Linux | `-Dlinux` |
| macOS | `-DmacOS` |
