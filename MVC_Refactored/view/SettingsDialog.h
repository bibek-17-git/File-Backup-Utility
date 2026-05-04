#pragma once
#include <gtk/gtk.h>
#include <string>
#include <functional>

class SettingsDialog {
public:
    using SaveCallback = std::function<void(const std::string&, bool, int, int, bool, bool)>;
    
    static void show(GtkWindow* parent, const std::string& dest, bool autoBackup,
                     int interval, int maxCopies, bool subfolders, bool hidden,
                     SaveCallback onSave) {
        GtkWidget* dialog = gtk_dialog_new_with_buttons(
            "Backup Settings", parent, GTK_DIALOG_MODAL,
            "_Save", GTK_RESPONSE_ACCEPT, "_Cancel", GTK_RESPONSE_REJECT, nullptr);
        gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 450);
        
        GtkWidget* grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
        gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
        gtk_container_set_border_width(GTK_CONTAINER(grid), 20);
        gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), grid);
        
        int row = 0;
        auto addLabel = [&](const char* text, int r) {
            GtkWidget* lbl = gtk_label_new(text);
            gtk_widget_set_halign(lbl, GTK_ALIGN_START);
            gtk_grid_attach(GTK_GRID(grid), lbl, 0, r, 1, 1);
        };
        
        addLabel("Backup Destination:", row);
        GtkWidget* destEntry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(destEntry), dest.c_str());
        gtk_grid_attach(GTK_GRID(grid), destEntry, 1, row++, 2, 1);
        
        addLabel("Auto Backup:", row);
        GtkWidget* autoSw = gtk_switch_new();
        gtk_switch_set_active(GTK_SWITCH(autoSw), autoBackup);
        gtk_grid_attach(GTK_GRID(grid), autoSw, 1, row++, 1, 1);
        
        addLabel("Interval (seconds):", row);
        GtkWidget* intervalSpin = gtk_spin_button_new_with_range(60, 86400, 60);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(intervalSpin), interval);
        gtk_grid_attach(GTK_GRID(grid), intervalSpin, 1, row++, 1, 1);
        
        addLabel("Max Copies:", row);
        GtkWidget* copiesSpin = gtk_spin_button_new_with_range(1, 100, 1);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(copiesSpin), maxCopies);
        gtk_grid_attach(GTK_GRID(grid), copiesSpin, 1, row++, 1, 1);
        
        addLabel("Include Subfolders:", row);
        GtkWidget* subSw = gtk_switch_new();
        gtk_switch_set_active(GTK_SWITCH(subSw), subfolders);
        gtk_grid_attach(GTK_GRID(grid), subSw, 1, row++, 1, 1);
        
        addLabel("Include Hidden:", row);
        GtkWidget* hiddenSw = gtk_switch_new();
        gtk_switch_set_active(GTK_SWITCH(hiddenSw), hidden);
        gtk_grid_attach(GTK_GRID(grid), hiddenSw, 1, row++, 1, 1);
        
        gtk_widget_show_all(dialog);
        
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT && onSave) {
            onSave(gtk_entry_get_text(GTK_ENTRY(destEntry)),
                   gtk_switch_get_active(GTK_SWITCH(autoSw)),
                   gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(intervalSpin)),
                   gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(copiesSpin)),
                   gtk_switch_get_active(GTK_SWITCH(subSw)),
                   gtk_switch_get_active(GTK_SWITCH(hiddenSw)));
        }
        gtk_widget_destroy(dialog);
    }
};
