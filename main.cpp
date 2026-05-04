#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <direct.h>
#include <dirent.h>
#include <unistd.h>

#ifdef _WIN32
    #define stat _stat
    #define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
    #define mkdir _mkdir
#endif

#define CONFIG_FILE "backup_config.txt"
#define LOG_FILE "backup_log.txt"
#define MAX_PATH 512
#define MAX_ITEMS 1000

typedef struct {
    char backup_destination[MAX_PATH];
    int auto_backup;
    int backup_interval;
    int max_copies;
    int backup_subfolders;
    int include_hidden;
    int show_notifications;
} BackupSettings;

BackupSettings settings = {
    .backup_destination = "C:\\Backups",
    .auto_backup = 1,
    .backup_interval = 300,
    .max_copies = 10,
    .backup_subfolders = 1,
    .include_hidden = 0,
    .show_notifications = 1
};

GtkWidget *window;
GtkWidget *progress_bar;
GtkWidget *status_label;
GtkListStore *items_list;
GtkWidget *treeview;
gboolean backup_running = FALSE;
guint timer_id = 0;
GtkWidget *dest_entry_global;

void load_settings();
void save_settings();
void add_file_to_list(const char *path);
void add_folder_to_list(const char *path, int recursive);
void update_status(const char *message, double progress);
void show_notification_msg(const char *title, const char *msg);
gboolean perform_backup(gpointer data);
gboolean auto_backup_timer(gpointer data);

void load_settings() {
    FILE *file = fopen(CONFIG_FILE, "r");
    if (!file) {
        save_settings();
        return;
    }

    char line[MAX_PATH];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;

        char *key = strtok(line, "=");
        char *value = strtok(NULL, "=");

        if (!key || !value) continue;

        if (strcmp(key, "destination") == 0)
            strcpy(settings.backup_destination, value);
        else if (strcmp(key, "auto_backup") == 0)
            settings.auto_backup = atoi(value);
        else if (strcmp(key, "interval") == 0)
            settings.backup_interval = atoi(value);
        else if (strcmp(key, "max_copies") == 0)
            settings.max_copies = atoi(value);
        else if (strcmp(key, "subfolders") == 0)
            settings.backup_subfolders = atoi(value);
        else if (strcmp(key, "hidden") == 0)
            settings.include_hidden = atoi(value);
    }
    fclose(file);

    mkdir(settings.backup_destination);
}

void save_settings() {
    FILE *file = fopen(CONFIG_FILE, "w");
    if (!file) return;

    fprintf(file, "destination=%s\n", settings.backup_destination);
    fprintf(file, "auto_backup=%d\n", settings.auto_backup);
    fprintf(file, "interval=%d\n", settings.backup_interval);
    fprintf(file, "max_copies=%d\n", settings.max_copies);
    fprintf(file, "subfolders=%d\n", settings.backup_subfolders);
    fprintf(file, "hidden=%d\n", settings.include_hidden);

    fclose(file);
}

void add_file_to_list(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return;

    if (!settings.include_hidden) {
        const char *name = strrchr(path, '\\');
        name = name ? name + 1 : path;
        if (name[0] == '.') return;
    }

    GtkTreeIter iter;
    gboolean exists = FALSE;

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(items_list), &iter)) {
        do {
            char *existing;
            gtk_tree_model_get(GTK_TREE_MODEL(items_list), &iter, 0, &existing, -1);
            if (strcmp(existing, path) == 0) {
                exists = TRUE;
                g_free(existing);
                break;
            }
            g_free(existing);
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(items_list), &iter));
    }

    if (!exists) {
        char size_str[32];
        if (st.st_size < 1024)
            sprintf(size_str, "%ld B", st.st_size);
        else if (st.st_size < 1024*1024)
            sprintf(size_str, "%.1f KB", st.st_size/1024.0);
        else if (st.st_size < 1024*1024*1024)
            sprintf(size_str, "%.1f MB", st.st_size/(1024.0*1024.0));
        else
            sprintf(size_str, "%.1f GB", st.st_size/(1024.0*1024.0*1024.0));

        char time_str[64];
        struct tm *tm = localtime(&st.st_mtime);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm);

        gtk_list_store_append(items_list, &iter);
        gtk_list_store_set(items_list, &iter,
                          0, path,
                          1, size_str,
                          2, time_str,
                          -1);
    }
}

void add_folder_to_list(const char *path, int recursive) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    char full_path[MAX_PATH];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s\\%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            if (recursive)
                add_folder_to_list(full_path, recursive);
        } else {
            add_file_to_list(full_path);
        }
    }
    closedir(dir);
}

void on_select_files_folders(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select Files and Folders",
        GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Add", GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

    GtkWidget *recursive_check = gtk_check_button_new_with_label("Include subfolders recursively");
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), recursive_check);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gboolean recursive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(recursive_check));
        GSList *files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));

        for (GSList *iter = files; iter; iter = iter->next) {
            char *path = (char*)iter->data;
            struct stat st;

            if (stat(path, &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    add_folder_to_list(path, recursive && settings.backup_subfolders);
                } else {
                    add_file_to_list(path);
                }
            }
            g_free(path);
        }
        g_slist_free(files);

        int count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(items_list), NULL);
        char msg[100];
        sprintf(msg, "%d items ready for backup", count);
        update_status(msg, 0.0);
    }

    gtk_widget_destroy(dialog);
}

void on_select_backup_folder(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select Backup Destination Folder",
        GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Select", GTK_RESPONSE_ACCEPT,
        NULL);

    if (strlen(settings.backup_destination) > 0) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), settings.backup_destination);
    }

    gtk_file_chooser_set_create_folders(GTK_FILE_CHOOSER(dialog), TRUE);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (folder) {
            strcpy(settings.backup_destination, folder);

            if (dest_entry_global) {
                gtk_entry_set_text(GTK_ENTRY(dest_entry_global), folder);
            }

            mkdir(folder);

            char msg[200];
            sprintf(msg, "Backup folder set to: %s", folder);
            update_status(msg, 0.0);

            g_free(folder);
        }
    }

    gtk_widget_destroy(dialog);
}

void on_select_folder_only(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select Folder to Backup",
        GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Select", GTK_RESPONSE_ACCEPT,
        NULL);

    GtkWidget *recursive_check = gtk_check_button_new_with_label("Include subfolders");
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), recursive_check);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gboolean recursive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(recursive_check));
        char *folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        if (folder) {
            add_folder_to_list(folder, recursive && settings.backup_subfolders);
            g_free(folder);
        }

        int count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(items_list), NULL);
        char msg[100];
        sprintf(msg, "%d items ready for backup", count);
        update_status(msg, 0.0);
    }

    gtk_widget_destroy(dialog);
}

void on_remove_items(GtkWidget *widget, gpointer data) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);

        int count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(items_list), NULL);
        char msg[100];
        sprintf(msg, "%d items remaining", count);
        update_status(msg, 0.0);
    } else {
        show_notification_msg("Notice", "Please select an item to remove");
    }
}

void on_clear_all(GtkWidget *widget, gpointer data) {
    gtk_list_store_clear(items_list);
    update_status("List cleared", 0.0);
}

void update_status(const char *message, double progress) {
    gtk_label_set_text(GTK_LABEL(status_label), message);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), progress);

    while (gtk_events_pending()) gtk_main_iteration();
}

void show_notification_msg(const char *title, const char *msg) {
    if (!settings.show_notifications) return;

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(window),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s", msg);
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

gboolean perform_backup(gpointer data) {
    if (backup_running) return G_SOURCE_REMOVE;

    GtkTreeIter iter;
    gboolean has_items = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(items_list), &iter);

    if (!has_items) {
        update_status("No items to backup", 0.0);
        return G_SOURCE_REMOVE;
    }

    backup_running = TRUE;

    int total = 0;
    GtkTreeIter count_iter = iter;
    do { total++; } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(items_list), &count_iter));

    int current = 0;
    int success = 0;

    char backup_dir[MAX_PATH];
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    sprintf(backup_dir, "%s\\Backup_%04d%02d%02d_%02d%02d",
            settings.backup_destination,
            tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
            tm->tm_hour, tm->tm_min);
    mkdir(backup_dir);

    char log_file[MAX_PATH];
    sprintf(log_file, "%s\\backup_log.txt", backup_dir);
    FILE *log = fopen(log_file, "w");
    if (log) {
        fprintf(log, "Backup started: %s", ctime(&now));
    }

    do {
        char *source_path;
        gtk_tree_model_get(GTK_TREE_MODEL(items_list), &iter, 0, &source_path, -1);

        current++;
        const char *filename = strrchr(source_path, '\\');
        filename = filename ? filename + 1 : source_path;

        char status[200];
        sprintf(status, "Backing up (%d/%d): %s", current, total, filename);
        update_status(status, (double)current/total);

        char dest_path[MAX_PATH];
        sprintf(dest_path, "%s\\%s", backup_dir, filename);

        FILE *src = fopen(source_path, "rb");
        if (src) {
            FILE *dst = fopen(dest_path, "wb");
            if (dst) {
                char buffer[8192];
                size_t bytes;
                while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                    fwrite(buffer, 1, bytes, dst);
                }
                fclose(dst);
                success++;

                if (log) {
                    fprintf(log, "✓ %s -> %s\n", source_path, dest_path);
                }
            }
            fclose(src);
        }

        g_free(source_path);

    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(items_list), &iter));

    if (log) {
        fprintf(log, "\nBackup completed: %d/%d files successful\n", success, total);
        fclose(log);
    }

    char final_msg[200];
    sprintf(final_msg, "Backup completed! %d/%d files backed up successfully", success, total);
    update_status(final_msg, 1.0);

    show_notification_msg("Backup Complete", final_msg);

    backup_running = FALSE;

    g_timeout_add_seconds(3, (GSourceFunc)gtk_progress_bar_set_fraction, GINT_TO_POINTER(0));

    return G_SOURCE_REMOVE;
}

gboolean auto_backup_timer(gpointer data) {
    if (settings.auto_backup && !backup_running) {
        g_idle_add(perform_backup, NULL);
    }
    return G_SOURCE_CONTINUE;
}

void on_backup_now(GtkWidget *widget, gpointer data) {
    if (backup_running) {
        show_notification_msg("Busy", "Backup already in progress");
        return;
    }
    g_idle_add(perform_backup, NULL);
}

void on_settings(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Backup Settings",
        GTK_WINDOW(window),
        GTK_DIALOG_MODAL,
        "_Save", GTK_RESPONSE_ACCEPT,
        "_Cancel", GTK_RESPONSE_REJECT,
        NULL);

    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 400);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 20);
    gtk_container_add(GTK_CONTAINER(content), grid);

    int row = 0;

    GtkWidget *dest_label = gtk_label_new("Backup Destination:");
    gtk_widget_set_halign(dest_label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), dest_label, 0, row, 1, 1);

    GtkWidget *dest_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_grid_attach(GTK_GRID(grid), dest_box, 1, row, 2, 1);

    dest_entry_global = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(dest_entry_global), settings.backup_destination);
    gtk_widget_set_hexpand(dest_entry_global, TRUE);
    gtk_box_pack_start(GTK_BOX(dest_box), dest_entry_global, TRUE, TRUE, 0);

    GtkWidget *browse_btn = gtk_button_new_with_label("Browse...");
    g_signal_connect(browse_btn, "clicked", G_CALLBACK(on_select_backup_folder), NULL);
    gtk_box_pack_start(GTK_BOX(dest_box), browse_btn, FALSE, FALSE, 0);
    row++;

    GtkWidget *auto_label = gtk_label_new("Auto Backup:");
    gtk_grid_attach(GTK_GRID(grid), auto_label, 0, row, 1, 1);

    GtkWidget *auto_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(auto_switch), settings.auto_backup);
    gtk_grid_attach(GTK_GRID(grid), auto_switch, 1, row, 1, 1);
    row++;

    GtkWidget *interval_label = gtk_label_new("Interval (seconds):");
    gtk_grid_attach(GTK_GRID(grid), interval_label, 0, row, 1, 1);

    GtkWidget *interval_spin = gtk_spin_button_new_with_range(60, 86400, 60);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(interval_spin), settings.backup_interval);
    gtk_grid_attach(GTK_GRID(grid), interval_spin, 1, row, 1, 1);
    row++;

    GtkWidget *copies_label = gtk_label_new("Max Copies:");
    gtk_grid_attach(GTK_GRID(grid), copies_label, 0, row, 1, 1);

    GtkWidget *copies_spin = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(copies_spin), settings.max_copies);
    gtk_grid_attach(GTK_GRID(grid), copies_spin, 1, row, 1, 1);
    row++;

    GtkWidget *sub_label = gtk_label_new("Include Subfolders:");
    gtk_grid_attach(GTK_GRID(grid), sub_label, 0, row, 1, 1);

    GtkWidget *sub_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(sub_switch), settings.backup_subfolders);
    gtk_grid_attach(GTK_GRID(grid), sub_switch, 1, row, 1, 1);
    row++;

    GtkWidget *hidden_label = gtk_label_new("Include Hidden:");
    gtk_grid_attach(GTK_GRID(grid), hidden_label, 0, row, 1, 1);

    GtkWidget *hidden_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(hidden_switch), settings.include_hidden);
    gtk_grid_attach(GTK_GRID(grid), hidden_switch, 1, row, 1, 1);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        strcpy(settings.backup_destination, gtk_entry_get_text(GTK_ENTRY(dest_entry_global)));
        settings.auto_backup = gtk_switch_get_active(GTK_SWITCH(auto_switch));
        settings.backup_interval = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(interval_spin));
        settings.max_copies = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(copies_spin));
        settings.backup_subfolders = gtk_switch_get_active(GTK_SWITCH(sub_switch));
        settings.include_hidden = gtk_switch_get_active(GTK_SWITCH(hidden_switch));

        save_settings();
        mkdir(settings.backup_destination);

        if (timer_id) {
            g_source_remove(timer_id);
            timer_id = 0;
        }
        if (settings.auto_backup) {
            timer_id = g_timeout_add_seconds(settings.backup_interval, auto_backup_timer, NULL);
        }

        update_status("Settings saved", 0.0);
    }

    gtk_widget_destroy(dialog);
}

void on_view_log(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Backup History",
        GTK_WINDOW(window),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL);

    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 400);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(scrolled), 10);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD);

    gtk_container_add(GTK_CONTAINER(scrolled), textview);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                      scrolled, TRUE, TRUE, 0);

    DIR *dir = opendir(settings.backup_destination);
    if (dir) {
        struct dirent *entry;
        char latest_log[MAX_PATH] = "";
        time_t latest_time = 0;

        while ((entry = readdir(dir)) != NULL) {
            if (strncmp(entry->d_name, "Backup_", 7) == 0) {
                char log_path[MAX_PATH];
                sprintf(log_path, "%s\\%s\\backup_log.txt", settings.backup_destination, entry->d_name);

                struct stat st;
                if (stat(log_path, &st) == 0 && st.st_mtime > latest_time) {
                    latest_time = st.st_mtime;
                    strcpy(latest_log, log_path);
                }
            }
        }
        closedir(dir);

        if (strlen(latest_log) > 0) {
            FILE *log = fopen(latest_log, "r");
            if (log) {
                GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
                char line[256];
                GString *content = g_string_new("");

                while (fgets(line, sizeof(line), log)) {
                    g_string_append(content, line);
                }
                fclose(log);

                gtk_text_buffer_set_text(buffer, content->str, -1);
                g_string_free(content, TRUE);
            }
        } else {
            GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
            gtk_text_buffer_set_text(buffer, "No backup logs found", -1);
        }
    }

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void create_main_window() {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Smart Backup Utility");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), main_box);

    GtkWidget *header = gtk_label_new("SMART BACKUP UTILITY");
    gtk_box_pack_start(GTK_BOX(main_box), header, FALSE, FALSE, 0);

    progress_bar = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(main_box), progress_bar, FALSE, FALSE, 0);

    status_label = gtk_label_new("Ready");
    gtk_widget_set_halign(status_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(main_box), status_label, FALSE, FALSE, 0);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(scrolled, TRUE);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_pack_start(GTK_BOX(main_box), scrolled, TRUE, TRUE, 0);

    items_list = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(items_list));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
        "File/Folder", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    column = gtk_tree_view_column_new_with_attributes(
        "Size", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    column = gtk_tree_view_column_new_with_attributes(
        "Modified", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    gtk_container_add(GTK_CONTAINER(scrolled), treeview);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(main_box), button_box, FALSE, FALSE, 0);

    GtkWidget *button;

    button = gtk_button_new_with_label("Add Files/Folders");
    g_signal_connect(button, "clicked", G_CALLBACK(on_select_files_folders), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);

    button = gtk_button_new_with_label("Add Folder Only");
    g_signal_connect(button, "clicked", G_CALLBACK(on_select_folder_only), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);

    button = gtk_button_new_with_label("Remove Selected");
    g_signal_connect(button, "clicked", G_CALLBACK(on_remove_items), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);

    button = gtk_button_new_with_label("Clear All");
    g_signal_connect(button, "clicked", G_CALLBACK(on_clear_all), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);

    GtkWidget *action_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(action_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(action_box, 10);
    gtk_box_pack_start(GTK_BOX(main_box), action_box, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("Start Backup");
    gtk_widget_set_size_request(button, 140, 40);
    g_signal_connect(button, "clicked", G_CALLBACK(on_backup_now), NULL);
    gtk_box_pack_start(GTK_BOX(action_box), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("Settings");
    gtk_widget_set_size_request(button, 140, 40);
    g_signal_connect(button, "clicked", G_CALLBACK(on_settings), NULL);
    gtk_box_pack_start(GTK_BOX(action_box), button, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("View Log");
    gtk_widget_set_size_request(button, 140, 40);
    g_signal_connect(button, "clicked", G_CALLBACK(on_view_log), NULL);
    gtk_box_pack_start(GTK_BOX(action_box), button, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
}

int main(int argc, char *argv[]) {
    g_setenv("GSETTINGS_SCHEMA_DIR", "C:\\msys64\\mingw64\\share\\glib-2.0\\schemas", FALSE);
    g_setenv("PATH", "C:\\msys64\\mingw64\\bin", FALSE);

    gtk_init(&argc, &argv);

    load_settings();
    create_main_window();

    if (settings.auto_backup && timer_id == 0) {
        timer_id = g_timeout_add_seconds(settings.backup_interval, auto_backup_timer, NULL);
    }

    gtk_main();
    return 0;
}
