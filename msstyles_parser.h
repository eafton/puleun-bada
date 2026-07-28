#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtkstyle.h>

#ifndef MSSTYLES_PARSER_H
#define MSSTYLES_PARSER_H

GKeyFile *msstyles_load_theme_ini(const gchar *msstyles_path, const gchar *color_scheme, const gchar *font_size);
gchar *msstyles_extract_resource(const gchar *msstyles_path, const gchar *resource_name);
gchar *msstyles_extract_resource_data(const gchar *msstyles_path, const gchar *resource_name, gsize *out_size);
gchar *msstyles_find_section(GKeyFile *ini, const gchar *section);
gboolean msstyles_parse_color(GKeyFile *ini, const gchar *section, const gchar *key, GdkColor *color);
gboolean msstyles_get_sys_color(GKeyFile *ini, const gchar *name, GdkColor *out);
gchar *msstyles_get_image_path(GKeyFile *ini, const gchar *section, const gchar *state);
gboolean msstyles_parse_margins(GKeyFile *ini, const gchar *section, const gchar *key, GtkBorder *margins);
gint msstyles_parse_int(GKeyFile *ini, const gchar *section, const gchar *key, gint default_value);
GdkPixbuf *msstyles_load_image(const gchar *msstyles_path, const gchar *image_name);
gchar **msstyles_parse_utf16_string_list(const gchar *data, gsize size, gsize *out_count);
void msstyles_set_current_theme(const gchar *path);
void msstyles_show_debug_dialog(void);
void msstyles_add_render_info(const gchar *info);

#endif
