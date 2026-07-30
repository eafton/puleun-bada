#include "pbui.h"
#include "pbui_style.h"
#include "pbui_rc_style.h"
#include <glib.h>

static void pbui_style_init(PbuiStyle *style);
static void pbui_style_class_init(PbuiStyleClass *klass);
static void pbui_style_finalize(GObject *object);
static void pbui_style_init_from_rc(GtkStyle *style, GtkRcStyle *rc_style);
static void pbui_draw_box(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height);
static void pbui_draw_flat_box(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height);
static void pbui_draw_check(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height);
static void pbui_draw_option(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height);
static void pbui_draw_shadow(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height);
static void pbui_draw_slider(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height, GtkOrientation orientation);
static void pbui_draw_handle(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height, GtkOrientation orientation);
static void pbui_draw_tab(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height);
static void pbui_draw_arrow(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, GtkArrowType arrow_type, gboolean fill, gint x, gint y, gint width, gint height);
static void pbui_draw_hline(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x1, gint x2, gint y);
static void pbui_draw_vline(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y1, gint y2);
static void pbui_draw_expander(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, GtkExpanderStyle expander_style);
static void pbui_draw_resize_grip(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, GdkWindowEdge edge, gint x, gint y, gint width, gint height);
static void pbui_draw_focus(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height);
static void pbui_draw_box_gap(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height, GtkPositionType gap_side, gint gap_x, gint gap_width);
static void pbui_draw_extension(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height, GtkPositionType gap_side);
static void pbui_draw_layout(GtkStyle *style, GdkWindow *window, GtkStateType state_type, gboolean use_text, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, PangoLayout *layout);

static GtkStyleClass *parent_class = NULL;

GType pbui_type_style = 0;

void pbui_style_register_type(GTypeModule *module) {
    static const GTypeInfo object_info = {sizeof(PbuiStyleClass), (GBaseInitFunc)NULL, (GBaseFinalizeFunc)NULL, (GClassInitFunc)pbui_style_class_init, NULL, NULL, sizeof(PbuiStyle), 0, (GInstanceInitFunc)pbui_style_init, NULL};

    pbui_type_style = g_type_module_register_type(module, GTK_TYPE_STYLE, "PbuiStyle", &object_info, 0);
}

static void pbui_style_init(PbuiStyle *style) {
    style->gtk3_theme_path = NULL;
    style->integer_scale = 1;
    style->parser = NULL;
}

static void pbui_style_finalize(GObject *object) {
    PbuiStyle *style = PBUI_STYLE(object);

    if (style->gtk3_theme_path) {
        g_free(style->gtk3_theme_path);
        style->gtk3_theme_path = NULL;
    }

    if (style->parser) {
        g_object_unref(style->parser);
        style->parser = NULL;
    }

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void pbui_style_init_from_rc(GtkStyle *style, GtkRcStyle *rc_style) {
    PbuiStyle *pbui_style = PBUI_STYLE(style);
    PbuiRcStyle *pbui_rc_style = PBUI_RC_STYLE(rc_style);

    parent_class->init_from_rc(style, rc_style);

    if (pbui_rc_style->gtk3_theme_path)
        pbui_style->gtk3_theme_path = g_strdup(pbui_rc_style->gtk3_theme_path);

    pbui_style->integer_scale = pbui_rc_style->integer_scale;

    g_debug("[PBUI] init_from_rc: gtk3_theme_path=%s integer_scale=%d", pbui_style->gtk3_theme_path, pbui_style->integer_scale);

    if (pbui_style->gtk3_theme_path) {
        gchar *css_path = g_build_filename(pbui_style->gtk3_theme_path, "gtk-3.0", "gtk.css", NULL);
        g_debug("[PBUI] Loading CSS from: %s", css_path);
        pbui_style->parser = pbui_css_parser_new();
        pbui_css_parser_set_integer_scale(pbui_style->parser, pbui_style->integer_scale);
        gboolean parsed = pbui_css_parser_parse_file(pbui_style->parser, css_path);
        g_debug("[PBUI] CSS parsed: %s", parsed ? "success" : "failed");
        g_free(css_path);
    }
}

static void pbui_draw_box(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height) {
    PbuiStyle *pbui_style = PBUI_STYLE(style);
    cairo_t *cr;
    PBUICSSRule *rule = NULL;
    const gchar *selector = NULL;

    g_debug("[PBUI] draw_box: detail=%s state=%d shadow=%d", detail ? detail : "(null)", state_type, shadow_type);

    if (!pbui_style->parser) {
        parent_class->draw_box(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
        return;
    }

    if (detail) {
        if (strcmp(detail, "button") == 0 || strcmp(detail, "buttondefault") == 0) {
            if (state_type == GTK_STATE_PRELIGHT) {
                selector = "button:hover";
            } else if (state_type == GTK_STATE_ACTIVE) {
                selector = "button:checked";
            } else if (state_type == GTK_STATE_INSENSITIVE) {
                selector = "button:disabled";
            } else {
                selector = "button";
            }
        } else if (strcmp(detail, "toolbar") == 0) {
            selector = "toolbar.primary-toolbar";
        } else if (strcmp(detail, "menubar") == 0) {
            selector = "menubar";
        } else if (strcmp(detail, "menuitem") == 0) {
            if (state_type == GTK_STATE_PRELIGHT) {
                selector = "menuitem:hover";
            } else {
                selector = "menuitem";
            }
        } else if (strcmp(detail, "entry") == 0) {
            if (state_type == GTK_STATE_INSENSITIVE) {
                selector = "entry:disabled";
            } else {
                selector = "entry";
            }
        }
    }

    if (selector) {
        g_debug("[PBUI] Looking up selector: %s", selector);
        rule = pbui_css_parser_lookup_rule(pbui_style->parser, selector);
        g_debug("[PBUI] Rule found: %s", rule ? "yes" : "no");
    }

    if (rule) {
        cr = gdk_cairo_create(window);

        if (area) {
            gdk_cairo_rectangle(cr, area);
            cairo_clip(cr);
        }

        g_debug("[PBUI] Rendering with rule: background=%p border=%p", rule->background, rule->border_color);

        if (rule->background) {
            g_debug("[PBUI] Gradient type=%d n_colors=%d", rule->background->type, rule->background->n_colors);
            pbui_cairo_draw_gradient_rect(cr, rule->background, x, y, width, height);
        }

        if (rule->border_color && rule->border_width > 0) {
            pbui_cairo_draw_border(cr, rule->border_color, rule->border_width, rule->radius, x, y, width, height);
        }

        cairo_destroy(cr);
    } else {
        g_debug("[PBUI] No rule found, falling back to parent");
        parent_class->draw_box(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
    }
}

static void pbui_draw_flat_box(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height) {
    PbuiStyle *pbui_style = PBUI_STYLE(style);
    cairo_t *cr;
    PBUICSSRule *rule = NULL;
    const gchar *selector = NULL;

    g_debug("[PBUI] draw_flat_box: detail=%s state=%d shadow=%d", detail ? detail : "(null)", state_type, shadow_type);

    if (!pbui_style->parser) {
        parent_class->draw_flat_box(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
        return;
    }

    if (detail) {
        if (strcmp(detail, "toolbar") == 0) {
            selector = "toolbar.primary-toolbar";
        } else if (strcmp(detail, "base") == 0 || strcmp(detail, "bg") == 0) {
            selector = "window";
        } else if (strcmp(detail, "tooltip") == 0) {
            selector = "tooltip";
        } else if (strcmp(detail, "menu") == 0) {
            selector = "menu";
        }
    }

    if (selector) {
        rule = pbui_css_parser_lookup_rule(pbui_style->parser, selector);
    }

    if (rule) {
        cr = gdk_cairo_create(window);

        if (area) {
            gdk_cairo_rectangle(cr, area);
            cairo_clip(cr);
        }

        if (rule->background) {
            pbui_cairo_draw_gradient_rect(cr, rule->background, x, y, width, height);
        }

        cairo_destroy(cr);
    } else {
        parent_class->draw_flat_box(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
    }
}

static void pbui_draw_check(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height) {
    PbuiStyle *pbui_style = PBUI_STYLE(style);
    cairo_t *cr;
    gboolean checked = (shadow_type == GTK_SHADOW_IN || shadow_type == GTK_SHADOW_ETCHED_IN);
    PBUICSSRule *rule = NULL;
    const gchar *selector = NULL;

    g_debug("[PBUI] draw_check: detail=%s state=%d shadow=%d", detail ? detail : "(null)", state_type, shadow_type);

    if (!pbui_style->parser) {
        parent_class->draw_check(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
        return;
    }

    if (checked) {
        selector = "check:checked";
    } else {
        selector = "check";
    }

    if (selector) {
        rule = pbui_css_parser_lookup_rule(pbui_style->parser, selector);
    }

    cr = gdk_cairo_create(window);

    if (area) {
        gdk_cairo_rectangle(cr, area);
        cairo_clip(cr);
    }

    if (rule && rule->background) {
        pbui_cairo_draw_gradient_rect(cr, rule->background, x, y, width, height);
    }

    if (rule && rule->border_color && rule->border_width > 0) {
        pbui_cairo_draw_border(cr, rule->border_color, rule->border_width, rule->radius, x, y, width, height);
    }

    if (checked) {
        cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
        cairo_set_line_width(cr, 2.0);
        cairo_move_to(cr, x + 3, y + height / 2);
        cairo_line_to(cr, x + width / 2, y + height - 3);
        cairo_line_to(cr, x + width - 3, y + 3);
        cairo_stroke(cr);
    }

    cairo_destroy(cr);
}

static void pbui_draw_option(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height) {
    PbuiStyle *pbui_style = PBUI_STYLE(style);
    cairo_t *cr;
    gboolean checked = (shadow_type == GTK_SHADOW_IN || shadow_type == GTK_SHADOW_ETCHED_IN);
    gdouble cx, cy, radius;
    PBUICSSRule *rule = NULL;
    const gchar *selector = NULL;

    g_debug("[PBUI] draw_option: detail=%s state=%d shadow=%d", detail ? detail : "(null)", state_type, shadow_type);

    if (!pbui_style->parser) {
        parent_class->draw_option(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
        return;
    }

    if (checked) {
        selector = "radio:checked";
    } else {
        selector = "radio";
    }

    if (selector) {
        rule = pbui_css_parser_lookup_rule(pbui_style->parser, selector);
    }

    cr = gdk_cairo_create(window);

    if (area) {
        gdk_cairo_rectangle(cr, area);
        cairo_clip(cr);
    }

    if (rule && rule->background) {
        pbui_cairo_draw_gradient_rect(cr, rule->background, x, y, width, height);
    }

    if (rule && rule->border_color && rule->border_width > 0) {
        pbui_cairo_draw_border(cr, rule->border_color, rule->border_width, rule->radius, x, y, width, height);
    }

    cx = x + width / 2.0;
    cy = y + height / 2.0;
    radius = MIN(width, height) / 3.0;

    if (checked) {
        cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
        cairo_arc(cr, cx, cy, radius, 0, 2 * G_PI);
        cairo_fill(cr);
    }

    cairo_destroy(cr);
}

static void pbui_draw_shadow(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height) {
    g_debug("[PBUI] draw_shadow: detail=%s state=%d shadow=%d", detail ? detail : "(null)", state_type, shadow_type);
    parent_class->draw_shadow(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
}

static void pbui_draw_slider(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height, GtkOrientation orientation) {
    g_debug("[PBUI] draw_slider: detail=%s state=%d shadow=%d orientation=%d", detail ? detail : "(null)", state_type, shadow_type, orientation);
    parent_class->draw_slider(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height, orientation);
}

static void pbui_draw_handle(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height, GtkOrientation orientation) {
    g_debug("[PBUI] draw_handle: detail=%s state=%d shadow=%d orientation=%d", detail ? detail : "(null)", state_type, shadow_type, orientation);
    parent_class->draw_handle(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height, orientation);
}

static void pbui_draw_tab(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height) {
    g_debug("[PBUI] draw_tab: detail=%s state=%d shadow=%d", detail ? detail : "(null)", state_type, shadow_type);
    parent_class->draw_tab(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
}

static void pbui_draw_arrow(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, GtkArrowType arrow_type, gboolean fill, gint x, gint y, gint width, gint height) {
    g_debug("[PBUI] draw_arrow: detail=%s state=%d shadow=%d arrow=%d", detail ? detail : "(null)", state_type, shadow_type, arrow_type);
    parent_class->draw_arrow(style, window, state_type, shadow_type, area, widget, detail, arrow_type, fill, x, y, width, height);
}

static void pbui_draw_hline(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x1, gint x2, gint y) {
    g_debug("[PBUI] draw_hline: detail=%s state=%d", detail ? detail : "(null)", state_type);
    parent_class->draw_hline(style, window, state_type, area, widget, detail, x1, x2, y);
}

static void pbui_draw_vline(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y1, gint y2) {
    g_debug("[PBUI] draw_vline: detail=%s state=%d", detail ? detail : "(null)", state_type);
    parent_class->draw_vline(style, window, state_type, area, widget, detail, x, y1, y2);
}

static void pbui_draw_expander(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, GtkExpanderStyle expander_style) {
    g_debug("[PBUI] draw_expander: detail=%s state=%d expander=%d", detail ? detail : "(null)", state_type, expander_style);
    parent_class->draw_expander(style, window, state_type, area, widget, detail, x, y, expander_style);
}

static void pbui_draw_resize_grip(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, GdkWindowEdge edge, gint x, gint y, gint width, gint height) {
    g_debug("[PBUI] draw_resize_grip: detail=%s state=%d edge=%d", detail ? detail : "(null)", state_type, edge);
    parent_class->draw_resize_grip(style, window, state_type, area, widget, detail, edge, x, y, width, height);
}

static void pbui_draw_focus(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height) {
    g_debug("[PBUI] draw_focus: detail=%s state=%d", detail ? detail : "(null)", state_type);
    parent_class->draw_focus(style, window, state_type, area, widget, detail, x, y, width, height);
}

static void pbui_draw_box_gap(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height, GtkPositionType gap_side, gint gap_x, gint gap_width) {
    g_debug("[PBUI] draw_box_gap: detail=%s state=%d shadow=%d gap_side=%d", detail ? detail : "(null)", state_type, shadow_type, gap_side);
    parent_class->draw_box_gap(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height, gap_side, gap_x, gap_width);
}

static void pbui_draw_extension(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height, GtkPositionType gap_side) {
    g_debug("[PBUI] draw_extension: detail=%s state=%d shadow=%d gap_side=%d", detail ? detail : "(null)", state_type, shadow_type, gap_side);
    parent_class->draw_extension(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height, gap_side);
}

static void pbui_draw_layout(GtkStyle *style, GdkWindow *window, GtkStateType state_type, gboolean use_text, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, PangoLayout *layout) {
    g_debug("[PBUI] draw_layout: detail=%s state=%d", detail ? detail : "(null)", state_type);
    parent_class->draw_layout(style, window, state_type, use_text, area, widget, detail, x, y, layout);
}

static void pbui_style_class_init(PbuiStyleClass *klass) {
    GtkStyleClass *style_class = GTK_STYLE_CLASS(klass);
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = pbui_style_finalize;
    style_class->init_from_rc = pbui_style_init_from_rc;
    style_class->draw_box = pbui_draw_box;
    style_class->draw_flat_box = pbui_draw_flat_box;
    style_class->draw_check = pbui_draw_check;
    style_class->draw_option = pbui_draw_option;
    style_class->draw_shadow = pbui_draw_shadow;
    style_class->draw_slider = pbui_draw_slider;
    style_class->draw_handle = pbui_draw_handle;
    style_class->draw_tab = pbui_draw_tab;
    style_class->draw_arrow = pbui_draw_arrow;
    style_class->draw_hline = pbui_draw_hline;
    style_class->draw_vline = pbui_draw_vline;
    style_class->draw_expander = pbui_draw_expander;
    style_class->draw_resize_grip = pbui_draw_resize_grip;
    style_class->draw_focus = pbui_draw_focus;
    style_class->draw_box_gap = pbui_draw_box_gap;
    style_class->draw_extension = pbui_draw_extension;
    style_class->draw_layout = pbui_draw_layout;
}
