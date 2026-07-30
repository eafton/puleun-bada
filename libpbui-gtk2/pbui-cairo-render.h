#ifndef PBUI_CAIRO_RENDER_H
#define PBUI_CAIRO_RENDER_H

#include <cairo.h>
#include <glib.h>
#include <glib-object.h>
#include "pbui-css-parser.h"

#include "pbui-gtk2-export.h"

G_BEGIN_DECLS

PBUI_GTK2_EXPORT void pbui_cairo_blur_image(cairo_surface_t* surface, gint radius);
PBUI_GTK2_EXPORT cairo_surface_t* pbui_cairo_create_surface_from_svg(const gchar* filename, gint width, gint height);
PBUI_GTK2_EXPORT void pbui_cairo_apply_theme_scaling(cairo_t* cr, gdouble scale);
PBUI_GTK2_EXPORT void pbui_cairo_draw_gradient_rect(cairo_t* cr, PBUIGradient* gradient, gint x, gint y, gint width, gint height);
PBUI_GTK2_EXPORT void pbui_cairo_draw_solid_rect(cairo_t* cr, PBUIColor* color, gint x, gint y, gint width, gint height);
PBUI_GTK2_EXPORT void pbui_cairo_draw_border(cairo_t* cr, PBUIColor* color, gdouble width, gdouble radius, gint x, gint y, gint w, gint h);
PBUI_GTK2_EXPORT void pbui_cairo_draw_rounded_rect(cairo_t* cr, gdouble radius, gint x, gint y, gint w, gint h);

G_END_DECLS

#endif
