#include "pbui-cairo-render.h"
#include <cairo.h>
#include <glib.h>
#include <math.h>

void pbui_cairo_blur_image(cairo_surface_t* surface, gint radius)
{
    if (!surface || radius <= 0) {
        return;
    }
}

cairo_surface_t* pbui_cairo_create_surface_from_svg(const gchar* filename, gint width, gint height)
{
    if (!filename || width <= 0 || height <= 0) {
        return NULL;
    }
    return NULL;
}

void pbui_cairo_apply_theme_scaling(cairo_t* cr, gdouble scale)
{
    if (!cr || scale <= 0.0) {
        return;
    }
}

void pbui_cairo_draw_rounded_rect(cairo_t* cr, gdouble radius, gint x, gint y, gint w, gint h)
{
    if (!cr) {
        return;
    }

    if (radius < 0.1) {
        cairo_rectangle(cr, x, y, w, h);
        return;
    }

    radius = MIN(radius, MIN(w / 2.0, h / 2.0));

    cairo_move_to(cr, x + radius, y);
    cairo_arc(cr, x + w - radius, y + radius, radius, -G_PI / 2, 0);
    cairo_arc(cr, x + w - radius, y + h - radius, radius, 0, G_PI / 2);
    cairo_arc(cr, x + radius, y + h - radius, radius, G_PI / 2, G_PI);
    cairo_arc(cr, x + radius, y + radius, radius, G_PI, 3 * G_PI / 2);
    cairo_close_path(cr);
}

void pbui_cairo_draw_solid_rect(cairo_t* cr, PBUIColor* color, gint x, gint y, gint width, gint height)
{
    if (!cr || !color) {
        return;
    }

    cairo_save(cr);
    cairo_set_source_rgba(cr, color->red, color->green, color->blue, color->alpha);
    cairo_rectangle(cr, x, y, width, height);
    cairo_fill(cr);
    cairo_restore(cr);
}

void pbui_cairo_draw_gradient_rect(cairo_t* cr, PBUIGradient* gradient, gint x, gint y, gint width, gint height)
{
    if (!cr || !gradient) {
        return;
    }

    if (gradient->type == PBUI_GRADIENT_NONE && gradient->n_colors > 0) {
        pbui_cairo_draw_solid_rect(cr, &gradient->colors[0], x, y, width, height);
        return;
    }

    if (gradient->type == PBUI_GRADIENT_LINEAR && gradient->n_colors > 0) {
        cairo_save(cr);
        gint i;
        cairo_pattern_t* pattern = cairo_pattern_create_linear(x, y, x, y + height);

        for (i = 0; i < gradient->n_colors; i++) {
            gdouble offset = (gradient->n_colors > 1) ? (gdouble)i / (gradient->n_colors - 1) : 0.0;
            cairo_pattern_add_color_stop_rgba(pattern, offset,
                gradient->colors[i].red,
                gradient->colors[i].green,
                gradient->colors[i].blue,
                gradient->colors[i].alpha);
        }

        cairo_set_source(cr, pattern);
        cairo_rectangle(cr, x, y, width, height);
        cairo_fill(cr);
        cairo_pattern_destroy(pattern);
        cairo_restore(cr);
    }
}

void pbui_cairo_draw_border(cairo_t* cr, PBUIColor* color, gdouble width, gdouble radius, gint x, gint y, gint w, gint h)
{
    if (!cr || !color || width <= 0) {
        return;
    }

    cairo_save(cr);
    cairo_set_source_rgba(cr, color->red, color->green, color->blue, color->alpha);
    cairo_set_line_width(cr, width);

    pbui_cairo_draw_rounded_rect(cr, radius, x + width / 2, y + width / 2, w - width, h - width);
    cairo_stroke(cr);
    cairo_restore(cr);
}
