#include "msstyles_draw.h"
#include "msstyles_parser.h"

static gint msstyles_muldiv(gint a, gint b, gint c) {
    return (gint)(((glong)a * b + c / 2) / c);
}

static void msstyles_cairo_paint_region(cairo_t *cr, GdkPixbuf *pixbuf, gint sx, gint sy, gint sw, gint sh, gint dx, gint dy, gint dw, gint dh) {
    GdkPixbuf *scaled;
    GdkPixbuf *sub;
    double scale_x, scale_y;
    cairo_pattern_t *pattern;

    if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
        return;

    sub = gdk_pixbuf_new_subpixbuf(pixbuf, sx, sy, sw, sh);
    if (!sub)
        return;

    scale_x = (double)dw / (double)sw;
    scale_y = (double)dh / (double)sh;

    cairo_save(cr);
    cairo_translate(cr, dx, dy);
    cairo_scale(cr, scale_x, scale_y);
    gdk_cairo_set_source_pixbuf(cr, sub, 0, 0);

    pattern = cairo_get_source(cr);
    cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);

    cairo_rectangle(cr, 0, 0, sw, sh);
    cairo_fill(cr);
    cairo_restore(cr);

    g_object_unref(sub);
}

void msstyles_draw_9slice_region(cairo_t *cr, GdkPixbuf *pixbuf, GtkBorder *margins, gint src_x, gint src_y, gint src_w, gint src_h, gint x, gint y, gint width, gint height) {
    gint left, right, top, bottom;
    gint dst_center_w, dst_center_h;
    gint src_center_w, src_center_h;

    if (!pixbuf || !cr)
        return;

    if (!margins) {
        msstyles_cairo_paint_region(cr, pixbuf, src_x, src_y, src_w, src_h, x, y, width, height);
        return;
    }

    left = margins->left;
    right = margins->right;
    top = margins->top;
    bottom = margins->bottom;

    if (left == 0 && right == 0 && top == 0 && bottom == 0) {
        msstyles_cairo_paint_region(cr, pixbuf, src_x, src_y, src_w, src_h, x, y, width, height);
        return;
    }

    if (left + right > src_w) {
        gint total = left + right;
        left = msstyles_muldiv(left, src_w, total);
        right = src_w - left;
    }

    if (top + bottom > src_h) {
        gint total = top + bottom;
        top = msstyles_muldiv(top, src_h, total);
        bottom = src_h - top;
    }

    if (left + right > width) {
        gint total = left + right;
        left = msstyles_muldiv(left, width, total);
        right = width - left;
    }

    if (top + bottom > height) {
        gint total = top + bottom;
        top = msstyles_muldiv(top, height, total);
        bottom = height - top;
    }

    dst_center_w = width - left - right;
    dst_center_h = height - top - bottom;
    src_center_w = src_w - left - right;
    src_center_h = src_h - top - bottom;

    if (dst_center_w < 0)
        dst_center_w = 0;
    if (dst_center_h < 0)
        dst_center_h = 0;
    if (src_center_w < 0)
        src_center_w = 0;
    if (src_center_h < 0)
        src_center_h = 0;

    cairo_save(cr);

    if (left > 0 && top > 0)
        msstyles_cairo_paint_region(cr, pixbuf, src_x, src_y, left, top, x, y, left, top);

    if (right > 0 && top > 0)
        msstyles_cairo_paint_region(cr, pixbuf, src_x + src_w - right, src_y, right, top, x + width - right, y, right, top);

    if (left > 0 && bottom > 0)
        msstyles_cairo_paint_region(cr, pixbuf, src_x, src_y + src_h - bottom, left, bottom, x, y + height - bottom, left, bottom);

    if (right > 0 && bottom > 0)
        msstyles_cairo_paint_region(cr, pixbuf, src_x + src_w - right, src_y + src_h - bottom, right, bottom, x + width - right, y + height - bottom, right, bottom);

    if (dst_center_w > 0 && src_center_w > 0) {
        if (top > 0)
            msstyles_cairo_paint_region(cr, pixbuf, src_x + left, src_y, src_center_w, top, x + left, y, dst_center_w, top);
        if (bottom > 0)
            msstyles_cairo_paint_region(cr, pixbuf, src_x + left, src_y + src_h - bottom, src_center_w, bottom, x + left, y + height - bottom, dst_center_w, bottom);
    }

    if (dst_center_h > 0 && src_center_h > 0) {
        if (left > 0)
            msstyles_cairo_paint_region(cr, pixbuf, src_x, src_y + top, left, src_center_h, x, y + top, left, dst_center_h);
        if (right > 0)
            msstyles_cairo_paint_region(cr, pixbuf, src_x + src_w - right, src_y + top, right, src_center_h, x + width - right, y + top, right, dst_center_h);
    }

    if (dst_center_w > 0 && dst_center_h > 0 && src_center_w > 0 && src_center_h > 0)
        msstyles_cairo_paint_region(cr, pixbuf, src_x + left, src_y + top, src_center_w, src_center_h, x + left, y + top, dst_center_w, dst_center_h);

    cairo_restore(cr);
}

void msstyles_draw_9slice(cairo_t *cr, GdkPixbuf *pixbuf, GtkBorder *margins, gint x, gint y, gint width, gint height) {
    gint src_w, src_h;

    if (!pixbuf)
        return;

    src_w = gdk_pixbuf_get_width(pixbuf);
    src_h = gdk_pixbuf_get_height(pixbuf);

    msstyles_draw_9slice_region(cr, pixbuf, margins, 0, 0, src_w, src_h, x, y, width, height);
}

void msstyles_draw_sliced_state(cairo_t *cr, GdkPixbuf *pixbuf, GtkBorder *margins, gint image_count, gint state_index, gint x, gint y, gint width, gint height) {
    msstyles_draw_sliced_state_with_layout(cr, pixbuf, margins, image_count, state_index, 0, x, y, width, height);
}

void msstyles_draw_sliced_state_with_layout(cairo_t *cr, GdkPixbuf *pixbuf, GtkBorder *margins, gint image_count, gint state_index, gint imagelayout, gint x, gint y, gint width, gint height) {
    gint src_w, src_h;
    gint slice_w, slice_h;
    gint slice_x, slice_y;
    gint imagenum;

    if (!pixbuf)
        return;

    if (image_count <= 0)
        image_count = 1;

    src_w = gdk_pixbuf_get_width(pixbuf);
    src_h = gdk_pixbuf_get_height(pixbuf);

    imagenum = state_index;
    if (imagenum < 1)
        imagenum = 1;
    if (imagenum > image_count)
        imagenum = image_count;

    imagenum = imagenum - 1;

    if (imagelayout == 1) {
        slice_w = src_w / image_count;
        slice_h = src_h;
        slice_x = slice_w * imagenum;
        slice_y = 0;

        if (slice_w <= 0 || slice_x + slice_w > src_w)
            slice_w = src_w - slice_x;

        if (slice_w > 0)
            msstyles_draw_9slice_region(cr, pixbuf, margins, slice_x, slice_y, slice_w, slice_h, x, y, width, height);
    } else {
        slice_h = src_h / image_count;
        slice_w = src_w;
        slice_x = 0;
        slice_y = slice_h * imagenum;

        if (slice_h <= 0 || slice_y + slice_h > src_h)
            slice_h = src_h - slice_y;

        if (slice_h > 0)
            msstyles_draw_9slice_region(cr, pixbuf, margins, slice_x, slice_y, slice_w, slice_h, x, y, width, height);
    }
}

static void msstyles_gdkcolor_to_rgb(const GdkColor *c, double *r, double *g, double *b) {
    *r = c->red / 65535.0;
    *g = c->green / 65535.0;
    *b = c->blue / 65535.0;
}

void msstyles_draw_border_fill(cairo_t *cr, GKeyFile *ini, const gchar *section, gint x, gint y, gint width, gint height) {
    GdkColor fill_color, border_color, grad1, grad2;
    gint fill_type, border_size;
    gboolean has_fill, has_border, has_grad;
    double r, g, b;
    cairo_pattern_t *pattern;
    double r1, g1, b1, r2, g2, b2;

    if (!cr || !ini || !section)
        return;

    fill_type = msstyles_parse_int(ini, section, "FillType", 0);
    border_size = msstyles_parse_int(ini, section, "BorderSize", 1);

    has_fill = msstyles_parse_color(ini, section, "FillColor", &fill_color);
    has_border = msstyles_parse_color(ini, section, "BorderColor", &border_color);
    has_grad = msstyles_parse_color(ini, section, "GradientColor1", &grad1) && msstyles_parse_color(ini, section, "GradientColor2", &grad2);

    cairo_save(cr);
    cairo_rectangle(cr, x, y, width, height);
    cairo_clip(cr);

    if (fill_type == 1 || fill_type == 2) {
        if (!has_grad) {
            grad1 = has_fill ? fill_color : border_color;
            grad2 = has_border ? border_color : fill_color;
        }

        msstyles_gdkcolor_to_rgb(&grad1, &r1, &g1, &b1);
        msstyles_gdkcolor_to_rgb(&grad2, &r2, &g2, &b2);

        if (fill_type == 1)
            pattern = cairo_pattern_create_linear(x, y, x, y + height);
        else
            pattern = cairo_pattern_create_linear(x, y, x + width, y);

        cairo_pattern_add_color_stop_rgb(pattern, 0.0, r1, g1, b1);
        cairo_pattern_add_color_stop_rgb(pattern, 1.0, r2, g2, b2);

        cairo_set_source(cr, pattern);
        cairo_rectangle(cr, x, y, width, height);
        cairo_fill(cr);
        cairo_pattern_destroy(pattern);
    } else if (has_fill) {
        msstyles_gdkcolor_to_rgb(&fill_color, &r, &g, &b);
        cairo_set_source_rgb(cr, r, g, b);
        cairo_rectangle(cr, x, y, width, height);
        cairo_fill(cr);
    }

    if (has_border && border_size > 0) {
        msstyles_gdkcolor_to_rgb(&border_color, &r, &g, &b);
        cairo_set_source_rgb(cr, r, g, b);
        cairo_set_line_width(cr, border_size);
        cairo_rectangle(cr, x + border_size / 2.0, y + border_size / 2.0, width - border_size, height - border_size);
        cairo_stroke(cr);
    }

    cairo_restore(cr);
}
