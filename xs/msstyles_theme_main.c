#include <gmodule.h>
#include <gtk/gtk.h>

#include "msstyles_parser.h"
#include "msstyles_rc_style.h"
#include "msstyles_style.h"

static GtkWidget *debug_dialog = NULL;
static gchar *current_theme_path = NULL;
static guint key_snooper_id = 0;
static gchar *last_render_info = NULL;

static void show_debug_dialog(void) {
    GtkWidget *dialog;
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *close_button;
    gchar *color_data;
    gchar *size_data;
    gsize color_size;
    gsize size_size;
    gchar **color_names;
    gchar **size_names;
    gsize color_count;
    gsize size_count;
    gchar *color_str;
    gchar *size_str;
    gchar *info_text;
    gsize i;

    if (debug_dialog) {
        gtk_window_present(GTK_WINDOW(debug_dialog));
        return;
    }

    dialog = gtk_dialog_new_with_buttons("Theme Debug Info", NULL, GTK_DIALOG_MODAL, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);

    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), vbox, TRUE, TRUE, 0);

    color_names = NULL;
    size_names = NULL;
    color_str = g_strdup("None");
    size_str = g_strdup("None");

    if (current_theme_path) {
        color_data = msstyles_extract_resource_data(current_theme_path, "COLORNAMES", &color_size);
        size_data = msstyles_extract_resource_data(current_theme_path, "SIZENAMES", &size_size);

        if (color_data) {
            color_names = msstyles_parse_utf16_string_list(color_data, color_size, &color_count);
            g_free(color_data);
        }
        if (size_data) {
            size_names = msstyles_parse_utf16_string_list(size_data, size_size, &size_count);
            g_free(size_data);
        }

        if (color_names && color_count > 0) {
            GString *str = g_string_new(NULL);
            for (i = 0; i < color_count; i++) {
                if (i > 0)
                    g_string_append(str, ", ");
                g_string_append(str, color_names[i]);
            }
            g_free(color_str);
            color_str = g_string_free(str, FALSE);
            g_strfreev(color_names);
        }

        if (size_names && size_count > 0) {
            GString *str = g_string_new(NULL);
            for (i = 0; i < size_count; i++) {
                if (i > 0)
                    g_string_append(str, ", ");
                g_string_append(str, size_names[i]);
            }
            g_free(size_str);
            size_str = g_string_free(str, FALSE);
            g_strfreev(size_names);
        }
    }

    info_text = g_strdup_printf("Current theme: %s\n\nColor schemes: %s\n\nFont schemes: %s\n\nLast render: %s", current_theme_path ? current_theme_path : "None", color_str, size_str, last_render_info ? last_render_info : "None");

    label = gtk_label_new(info_text);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

    g_free(info_text);
    g_free(color_str);
    g_free(size_str);

    close_button = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);
    if (close_button)
        gtk_widget_grab_focus(close_button);

    g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
    g_signal_connect(dialog, "destroy", G_CALLBACK(gtk_widget_destroyed), &debug_dialog);

    debug_dialog = dialog;
    gtk_widget_show_all(dialog);
}

static gboolean key_snooper_handler(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if (event->type == GDK_KEY_PRESS) {
        if ((event->state & GDK_MOD1_MASK) && (event->state & GDK_SHIFT_MASK) && gdk_keyval_to_lower(event->keyval) == gdk_keyval_from_name("d")) {
            show_debug_dialog();
            return TRUE;
        }
    }
    return FALSE;
}

G_MODULE_EXPORT void theme_init(GTypeModule *module) {
    msstyles_rc_style_register_type(module);
    msstyles_style_register_type(module);
    key_snooper_id = gtk_key_snooper_install(key_snooper_handler, NULL);
}

void msstyles_set_current_theme(const gchar *path) {
    if (current_theme_path)
        g_free(current_theme_path);
    current_theme_path = g_strdup(path);
}

void msstyles_show_debug_dialog(void) {
    show_debug_dialog();
}

void msstyles_add_render_info(const gchar *info) {
    if (last_render_info)
        g_free(last_render_info);
    last_render_info = g_strdup(info);
}

G_MODULE_EXPORT void theme_exit(void) {
    if (key_snooper_id > 0) {
        gtk_key_snooper_remove(key_snooper_id);
        key_snooper_id = 0;
    }
    if (current_theme_path) {
        g_free(current_theme_path);
        current_theme_path = NULL;
    }
}

G_MODULE_EXPORT GtkRcStyle *theme_create_rc_style(void) {
    return GTK_RC_STYLE(g_object_new(MSSTYLES_TYPE_RC_STYLE, NULL));
}

G_MODULE_EXPORT const gchar *g_module_check_init(GModule *module);
const gchar *g_module_check_init(GModule *module) {
    return gtk_check_version(GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION - GTK_INTERFACE_AGE);
}
