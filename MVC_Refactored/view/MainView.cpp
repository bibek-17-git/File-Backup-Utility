#include "IMainView.h"
#include <gtk/gtk.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/stat.h>

#ifdef _WIN32
    #define stat _stat
#endif

class MainView : public IMainView {
private:
    GtkWidget* m_window;
    GtkWidget* m_progressBar;
    GtkWidget* m_statusLabel;
    GtkListStore* m_listStore;
    GtkWidget* m_treeView;
    GtkWidget* m_startButton;
    GtkTreeSelection* m_selection;
    
    FileSelectedCallback m_onFileSelected;
    RemoveSelectedCallback m_onRemoveSelected;
    ClearAllCallback m_onClearAll;
    StartBackupCallback m_onStartBackup;
    OpenSettingsCallback m_onOpenSettings;
    ViewLogCallback m_onViewLog;
    
    std::string formatSize(long bytes) {
        std::ostringstream oss;
        if (bytes < 1024) oss << bytes << " B";
        else if (bytes < 1024*1024) oss << (bytes/1024.0) << " KB";
        else if (bytes < 1024*1024*1024) oss << (bytes/(1024.0*1024.0)) << " MB";
        else oss << (bytes/(1024.0*1024.0*1024.0)) << " GB";
        return oss.str();
    }
    
    std::string lastModified(const std::string& path) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) return "";
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&st.st_mtime));
        return buf;
    }
    
    std::vector<std::string> showFileChooserDialog() {
        std::vector<std::string> paths;
        GtkWidget* dialog = gtk_file_chooser_dialog_new(
            "Select Files", GTK_WINDOW(m_window),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Add", GTK_RESPONSE_ACCEPT, nullptr);
        gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
        
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            GSList* files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
            for (GSList* it = files; it; it = it->next) {
                paths.push_back(static_cast<char*>(it->data));
                g_free(it->data);
            }
            g_slist_free(files);
        }
        gtk_widget_destroy(dialog);
        return paths;
    }
    
    std::vector<std::string> showFolderDialog() {
        std::vector<std::string> paths;
        GtkWidget* dialog = gtk_file_chooser_dialog_new(
            "Select Folder", GTK_WINDOW(m_window),
            GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Select", GTK_RESPONSE_ACCEPT, nullptr);
        
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char* folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            if (folder) {
                paths.push_back(folder);
                g_free(folder);
            }
        }
        gtk_widget_destroy(dialog);
        return paths;
    }
    
    static void onAddFilesClicked(GtkWidget*, gpointer data) {
        auto* self = static_cast<MainView*>(data);
        if (self->m_onFileSelected) {
            auto paths = self->showFileChooserDialog();
            if (!paths.empty()) self->m_onFileSelected(paths, false);
        }
    }
    
    static void onAddFolderClicked(GtkWidget*, gpointer data) {
        auto* self = static_cast<MainView*>(data);
        if (self->m_onFileSelected) {
            auto paths = self->showFolderDialog();
            if (!paths.empty()) self->m_onFileSelected(paths, true);
        }
    }
    
    static void onRemoveSelectedClicked(GtkWidget*, gpointer data) {
        auto* self = static_cast<MainView*>(data);
        if (self->m_onRemoveSelected) self->m_onRemoveSelected();
    }
    
    static void onClearAllClicked(GtkWidget*, gpointer data) {
        auto* self = static_cast<MainView*>(data);
        if (self->m_onClearAll) self->m_onClearAll();
    }
    
    static void onStartBackupClicked(GtkWidget*, gpointer data) {
        auto* self = static_cast<MainView*>(data);
        if (self->m_onStartBackup) self->m_onStartBackup();
    }
    
    static void onSettingsClicked(GtkWidget*, gpointer data) {
        auto* self = static_cast<MainView*>(data);
        if (self->m_onOpenSettings) self->m_onOpenSettings();
    }
    
    static void onViewLogClicked(GtkWidget*, gpointer data) {
        auto* self = static_cast<MainView*>(data);
        if (self->m_onViewLog) self->m_onViewLog();
    }
    
    void buildUI() {
        m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(m_window), "Smart Backup - MVC Edition");
        gtk_window_set_default_size(GTK_WINDOW(m_window), 900, 600);
        gtk_window_set_position(GTK_WINDOW(m_window), GTK_WIN_POS_CENTER);
        gtk_container_set_border_width(GTK_CONTAINER(m_window), 10);
        g_signal_connect(m_window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);
        
        GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_container_add(GTK_CONTAINER(m_window), mainBox);
        
        GtkWidget* header = gtk_label_new(NULL);
        char* markup = g_markup_printf_escaped(
            "<span size='x-large' weight='bold'>📦 SMART BACKUP UTILITY</span>\n"
            "<span size='small'>MVC + SOLID Architecture</span>", NULL);
        gtk_label_set_markup(GTK_LABEL(header), markup);
        g_free(markup);
        gtk_box_pack_start(GTK_BOX(mainBox), header, FALSE, FALSE, 0);
        
        m_progressBar = gtk_progress_bar_new();
        gtk_box_pack_start(GTK_BOX(mainBox), m_progressBar, FALSE, FALSE, 0);
        
        m_statusLabel = gtk_label_new("✓ Ready");
        gtk_widget_set_halign(m_statusLabel, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(mainBox), m_statusLabel, FALSE, FALSE, 0);
        
        GtkWidget* scrolled = gtk_scrolled_window_new(nullptr, nullptr);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_widget_set_hexpand(scrolled, TRUE);
        gtk_widget_set_vexpand(scrolled, TRUE);
        gtk_box_pack_start(GTK_BOX(mainBox), scrolled, TRUE, TRUE, 0);
        
        m_listStore = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        m_treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(m_listStore));
        m_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(m_treeView));
        
        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        
        GtkTreeViewColumn* col = gtk_tree_view_column_new_with_attributes(
            "File/Folder", renderer, "text", 0, nullptr);
        gtk_tree_view_column_set_expand(col, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(m_treeView), col);
        
        col = gtk_tree_view_column_new_with_attributes("Size", renderer, "text", 1, nullptr);
        gtk_tree_view_append_column(GTK_TREE_VIEW(m_treeView), col);
        
        col = gtk_tree_view_column_new_with_attributes("Modified", renderer, "text", 2, nullptr);
        gtk_tree_view_append_column(GTK_TREE_VIEW(m_treeView), col);
        
        gtk_container_add(GTK_CONTAINER(scrolled), m_treeView);
        
        GtkWidget* btnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_widget_set_halign(btnBox, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(mainBox), btnBox, FALSE, FALSE, 0);
        
        auto makeBtn = [&](const char* label, GCallback cb) {
            GtkWidget* b = gtk_button_new_with_label(label);
            g_signal_connect(b, "clicked", cb, this);
            gtk_box_pack_start(GTK_BOX(btnBox), b, TRUE, TRUE, 0);
        };
        
        makeBtn("📁 Add Files", G_CALLBACK(onAddFilesClicked));
        makeBtn("📂 Add Folder", G_CALLBACK(onAddFolderClicked));
        makeBtn("❌ Remove", G_CALLBACK(onRemoveSelectedClicked));
        makeBtn("🗑 Clear All", G_CALLBACK(onClearAllClicked));
        
        GtkWidget* actBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_halign(actBox, GTK_ALIGN_CENTER);
        gtk_widget_set_margin_top(actBox, 10);
        gtk_box_pack_start(GTK_BOX(mainBox), actBox, FALSE, FALSE, 0);
        
        m_startButton = gtk_button_new_with_label("▶ START BACKUP");
        gtk_widget_set_size_request(m_startButton, 160, 45);
        g_signal_connect(m_startButton, "clicked", G_CALLBACK(onStartBackupClicked), this);
        gtk_box_pack_start(GTK_BOX(actBox), m_startButton, FALSE, FALSE, 0);
        
        GtkWidget* settingsBtn = gtk_button_new_with_label("⚙ SETTINGS");
        gtk_widget_set_size_request(settingsBtn, 160, 45);
        g_signal_connect(settingsBtn, "clicked", G_CALLBACK(onSettingsClicked), this);
        gtk_box_pack_start(GTK_BOX(actBox), settingsBtn, FALSE, FALSE, 0);
        
        GtkWidget* logBtn = gtk_button_new_with_label("📋 VIEW LOG");
        gtk_widget_set_size_request(logBtn, 160, 45);
        g_signal_connect(logBtn, "clicked", G_CALLBACK(onViewLogClicked), this);
        gtk_box_pack_start(GTK_BOX(actBox), logBtn, FALSE, FALSE, 0);
    }
    
public:
    MainView() { buildUI(); }
    
    void show() override { gtk_widget_show_all(m_window); }
    
    void updateStatus(const std::string& message, double progress) override {
        gtk_label_set_text(GTK_LABEL(m_statusLabel), message.c_str());
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(m_progressBar), progress);
        while (gtk_events_pending()) gtk_main_iteration();
    }
    
    void updateFileList(const std::vector<std::string>& files) override {
        gtk_list_store_clear(m_listStore);
        for (const auto& file : files) {
            GtkTreeIter iter;
            gtk_list_store_append(m_listStore, &iter);
            
            struct stat st;
            long size = (stat(file.c_str(), &st) == 0) ? st.st_size : -1;
            std::string sizeStr = (size >= 0) ? formatSize(size) : "?";
            std::string modStr = lastModified(file);
            
            gtk_list_store_set(m_listStore, &iter, 0, file.c_str(), 
                               1, sizeStr.c_str(), 2, modStr.c_str(), -1);
        }
    }
    
    void showNotification(const std::string& title, const std::string& message) override {
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
            GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK, "%s", message.c_str());
        gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    
    void showError(const std::string& error) override {
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
            GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK, "%s", error.c_str());
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    
    void setBackupButtonEnabled(bool enabled) override {
        gtk_widget_set_sensitive(m_startButton, enabled);
    }
    
    std::string getSelectedItem() override {
        GtkTreeModel* model;
        GtkTreeIter iter;
        if (gtk_tree_selection_get_selected(m_selection, &model, &iter)) {
            char* path;
            gtk_tree_model_get(model, &iter, 0, &path, -1);
            std::string result(path);
            g_free(path);
            return result;
        }
        return "";
    }
    
    void onFileSelected(FileSelectedCallback callback) override { m_onFileSelected = callback; }
    void onRemoveSelected(RemoveSelectedCallback callback) override { m_onRemoveSelected = callback; }
    void onClearAll(ClearAllCallback callback) override { m_onClearAll = callback; }
    void onStartBackup(StartBackupCallback callback) override { m_onStartBackup = callback; }
    void onOpenSettings(OpenSettingsCallback callback) override { m_onOpenSettings = callback; }
    void onViewLog(ViewLogCallback callback) override { m_onViewLog = callback; }
};
