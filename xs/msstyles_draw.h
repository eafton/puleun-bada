#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#ifndef MSSTYLES_DRAW_H
#define MSSTYLES_DRAW_H

void msstyles_draw_9slice_region(cairo_t *cr, GdkPixbuf *pixbuf, GtkBorder *margins, gint src_x, gint src_y, gint src_w, gint src_h, gint x, gint y, gint width, gint height);
void msstyles_draw_9slice(cairo_t *cr, GdkPixbuf *pixbuf, GtkBorder *margins, gint x, gint y, gint width, gint height);
void msstyles_draw_sliced_state(cairo_t *cr, GdkPixbuf *pixbuf, GtkBorder *margins, gint image_count, gint state_index, gint x, gint y, gint width, gint height);
void msstyles_draw_sliced_state_with_layout(cairo_t *cr, GdkPixbuf *pixbuf, GtkBorder *margins, gint image_count, gint state_index, gint imagelayout, gint x, gint y, gint width, gint height);
void msstyles_draw_border_fill(cairo_t *cr, GKeyFile *ini, const gchar *section, gint x, gint y, gint width, gint height);

#endif
