#pragma once
#include "../model/ISettingsModel.h"
#include "../view/IMainView.h"

class SettingsController {
private:
    ISettingsModel* m_model;
    IMainView*      m_view;

    void showSettingsDialog();

public:
    SettingsController(ISettingsModel* model, IMainView* view);
};
