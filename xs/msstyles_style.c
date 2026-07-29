#include "msstyles_style.h"
#include "msstyles_draw.h"
#include "msstyles_parser.h"
#include "msstyles_rc_style.h"
#include <glib.h>
#include <string.h>

static void msstyles_style_init(MsStylesStyle *style);
static void msstyles_style_class_init(MsStylesStyleClass *klass);
static void msstyles_style_finalize(GObject *object);
static void msstyles_style_init_from_rc(GtkStyle *style, GtkRcStyle *rc_style);

static GtkStyleClass *parent_class = NULL;

GType msstyles_type_style = 0;

void msstyles_style_register_type(GTypeModule *module) {
    static const GTypeInfo object_info = {sizeof(MsStylesStyleClass), (GBaseInitFunc)NULL, (GBaseFinalizeFunc)NULL, (GClassInitFunc)msstyles_style_class_init, NULL, NULL, sizeof(MsStylesStyle), 0, (GInstanceInitFunc)msstyles_style_init, NULL};

    msstyles_type_style = g_type_module_register_type(module, GTK_TYPE_STYLE, "MsStylesStyle", &object_info, 0);
}

static void msstyles_style_init(MsStylesStyle *style) {
    style->msstyles_path = NULL;
    style->color_scheme = NULL;
    style->font_size = NULL;
    style->theme_ini = NULL;
}

static void msstyles_style_finalize(GObject *object) {
    MsStylesStyle *style = MSSTYLES_STYLE(object);

    if (style->msstyles_path) {
        g_free(style->msstyles_path);
        style->msstyles_path = NULL;
    }

    if (style->color_scheme) {
        g_free(style->color_scheme);
        style->color_scheme = NULL;
    }

    if (style->font_size) {
        g_free(style->font_size);
        style->font_size = NULL;
    }

    if (style->theme_ini) {
        g_key_file_unref(style->theme_ini);
        style->theme_ini = NULL;
    }

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void msstyles_apply_sys_colors(GtkStyle *style, GKeyFile *ini) {
    GdkColor c;
    gint i;

    if (!style || !ini)
        return;

    if (msstyles_get_sys_color(ini, "ButtonFace", &c)) {
        for (i = 0; i < 5; i++)
            style->bg[i] = c;
    }
    if (msstyles_get_sys_color(ini, "Window", &c)) {
        for (i = 0; i < 5; i++)
            style->base[i] = c;
    }
    if (msstyles_get_sys_color(ini, "ButtonText", &c)) {
        for (i = 0; i < 5; i++)
            style->fg[i] = c;
    }
    if (msstyles_get_sys_color(ini, "WindowText", &c)) {
        for (i = 0; i < 5; i++)
            style->text[i] = c;
    }
    if (msstyles_get_sys_color(ini, "Highlight", &c)) {
        style->bg[GTK_STATE_SELECTED] = c;
        style->base[GTK_STATE_SELECTED] = c;
    }
    if (msstyles_get_sys_color(ini, "HighlightText", &c)) {
        style->fg[GTK_STATE_SELECTED] = c;
        style->text[GTK_STATE_SELECTED] = c;
    }
}

static void msstyles_style_init_from_rc(GtkStyle *style, GtkRcStyle *rc_style) {
    MsStylesStyle *msstyles_style = MSSTYLES_STYLE(style);
    MsStylesRcStyle *msstyles_rc_style = MSSTYLES_RC_STYLE(rc_style);

    parent_class->init_from_rc(style, rc_style);

    if (msstyles_rc_style->msstyles_path)
        msstyles_style->msstyles_path = g_strdup(msstyles_rc_style->msstyles_path);

    if (msstyles_rc_style->color_scheme)
        msstyles_style->color_scheme = g_strdup(msstyles_rc_style->color_scheme);

    if (msstyles_rc_style->font_size)
        msstyles_style->font_size = g_strdup(msstyles_rc_style->font_size);

    if (msstyles_style->msstyles_path) {
        msstyles_set_current_theme(msstyles_style->msstyles_path);
        msstyles_style->theme_ini = msstyles_load_theme_ini(msstyles_style->msstyles_path, msstyles_style->color_scheme, msstyles_style->font_size);
        if (msstyles_style->theme_ini) {
            msstyles_apply_sys_colors(style, msstyles_style->theme_ini);
        }
    }
}

static gchar *msstyles_find_key_ci(GKeyFile *ini, const gchar *section, const gchar *key) {
    gchar **keys;
    gchar *actual_key = NULL;
    gint i;

    keys = g_key_file_get_keys(ini, section, NULL, NULL);
    if (keys) {
        for (i = 0; keys[i]; i++) {
            if (g_ascii_strcasecmp(keys[i], key) == 0) {
                actual_key = g_strdup(keys[i]);
                break;
            }
        }
        g_strfreev(keys);
    }
    return actual_key;
}

static gchar *msstyles_get_string_fallback(GKeyFile *ini, gchar **sections, const gchar *key) {
    gchar *val;
    gchar *real_key;
    gint i;
    for (i = 0; i < 3; i++) {
        if (!sections[i])
            continue;
        real_key = msstyles_find_key_ci(ini, sections[i], key);
        if (real_key) {
            val = g_key_file_get_string(ini, sections[i], real_key, NULL);
            g_free(real_key);
            if (val)
                return val;
        }
    }
    return NULL;
}

static gint msstyles_get_int_fallback(GKeyFile *ini, gchar **sections, const gchar *key, gint default_val) {
    gint i, val;
    GError *err;
    gchar *real_key;
    for (i = 0; i < 3; i++) {
        if (!sections[i])
            continue;
        real_key = msstyles_find_key_ci(ini, sections[i], key);
        if (real_key) {
            err = NULL;
            val = g_key_file_get_integer(ini, sections[i], real_key, &err);
            g_free(real_key);
            if (!err)
                return val;
            g_error_free(err);
        }
    }
    return default_val;
}

static gboolean msstyles_get_margins_fallback(GKeyFile *ini, gchar **sections, const gchar *key, GtkBorder *margins) {
    gint i;
    gchar *real_key;
    for (i = 0; i < 3; i++) {
        if (!sections[i])
            continue;
        real_key = msstyles_find_key_ci(ini, sections[i], key);
        if (real_key) {
            gboolean res = msstyles_parse_margins(ini, sections[i], real_key, margins);
            g_free(real_key);
            if (res)
                return TRUE;
        }
    }
    return FALSE;
}

static gboolean msstyles_resolve_part(MsStylesStyle *style, const gchar *class_name, const gchar *part_name, gchar **out_section, gchar **out_image_path, GtkBorder *out_margins, gint *out_image_count, gint *out_bgtype, gint *out_imagelayout) {
    gchar *combined;
    gchar *sections[3];
    gboolean margins_ok;

    out_margins->left = 0;
    out_margins->right = 0;
    out_margins->top = 0;
    out_margins->bottom = 0;

    if (!style->theme_ini)
        return FALSE;

    combined = g_strdup_printf("%s.%s", class_name, part_name);
    sections[0] = msstyles_find_section(style->theme_ini, combined);
    g_free(combined);

    sections[1] = msstyles_find_section(style->theme_ini, class_name);
    sections[2] = msstyles_find_section(style->theme_ini, "globals");

    if (!sections[0] && !sections[1]) {
        g_free(sections[2]);
        return FALSE;
    }

    *out_section = sections[0] ? g_strdup(sections[0]) : g_strdup(sections[1]);

    *out_bgtype = msstyles_get_int_fallback(style->theme_ini, sections, "BgType", 0);
    *out_image_count = msstyles_get_int_fallback(style->theme_ini, sections, "ImageCount", 1);
    *out_imagelayout = msstyles_get_int_fallback(style->theme_ini, sections, "ImageLayout", 0);

    margins_ok = msstyles_get_margins_fallback(style->theme_ini, sections, "SizingMargins", out_margins);

    if (!margins_ok)
        margins_ok = msstyles_get_margins_fallback(style->theme_ini, sections, "ContentMargins", out_margins);

    if (!margins_ok)
        margins_ok = msstyles_get_margins_fallback(style->theme_ini, sections, "Margins", out_margins);

    if (!margins_ok || out_margins->left < 0 || out_margins->right < 0 || out_margins->top < 0 || out_margins->bottom < 0) {
        out_margins->left = 0;
        out_margins->right = 0;
        out_margins->top = 0;
        out_margins->bottom = 0;
    }

    *out_image_path = msstyles_get_string_fallback(style->theme_ini, sections, "ImageFile");

    g_free(sections[0]);
    g_free(sections[1]);
    g_free(sections[2]);

    return TRUE;
}

static gboolean msstyles_render_part(MsStylesStyle *style, GdkWindow *window, GdkRectangle *area, const gchar *class_name, const gchar *part_name, gint state_index, gint x, gint y, gint width, gint height) {
    gchar *section;
    gchar *image_path;
    GtkBorder margins;
    gint image_count;
    gint bgtype;
    gint imagelayout;
    cairo_t *cr;
    gboolean drew;
    GdkPixbuf *pixbuf;

    section = NULL;
    image_path = NULL;
    drew = FALSE;

    if (!msstyles_resolve_part(style, class_name, part_name, &section, &image_path, &margins, &image_count, &bgtype, &imagelayout))
        return FALSE;

    cr = gdk_cairo_create(window);

    if (area) {
        gdk_cairo_rectangle(cr, area);
        cairo_clip(cr);
    }

    if (bgtype == 1) {
        msstyles_draw_border_fill(cr, style->theme_ini, section, x, y, width, height);
        drew = TRUE;
    } else if (bgtype == 2) {
        drew = TRUE;
    } else if (image_path) {
        pixbuf = msstyles_load_image(style->msstyles_path, image_path);
        if (pixbuf) {
            msstyles_draw_sliced_state_with_layout(cr, pixbuf, &margins, image_count, state_index, imagelayout, x, y, width, height);
            g_object_unref(pixbuf);
            drew = TRUE;
        }
    }

    cairo_destroy(cr);
    g_free(section);
    g_free(image_path);

    return drew;
}

static gint msstyles_pushbutton_state(GtkStateType state_type, GtkShadowType shadow_type) {
    if (state_type == GTK_STATE_INSENSITIVE)
        return 4;
    if (state_type == GTK_STATE_PRELIGHT)
        return 2;
    if (state_type == GTK_STATE_ACTIVE)
        return 3;
    if (shadow_type == GTK_SHADOW_IN)
        return 5;
    return 1;
}

static gint msstyles_checkmark_state(GtkStateType state_type, gboolean checked) {
    gint base;

    base = checked ? 5 : 1;
    if (state_type == GTK_STATE_INSENSITIVE)
        return base + 3;
    if (state_type == GTK_STATE_PRELIGHT)
        return base + 1;
    if (state_type == GTK_STATE_ACTIVE)
        return base + 2;
    return base;
}

static gint msstyles_tabitem_state(GtkStateType state_type, gboolean selected) {
    if (selected)
        return 3;
    if (state_type == GTK_STATE_INSENSITIVE)
        return 4;
    if (state_type == GTK_STATE_PRELIGHT)
        return 2;
    return 1;
}

static gint msstyles_thumb_state(GtkStateType state_type) {
    if (state_type == GTK_STATE_INSENSITIVE)
        return 5;
    if (state_type == GTK_STATE_PRELIGHT)
        return 2;
    if (state_type == GTK_STATE_ACTIVE)
        return 3;
    return 1;
}

static gint msstyles_arrow_state(GtkStateType state_type) {
    if (state_type == GTK_STATE_INSENSITIVE)
        return 4;
    if (state_type == GTK_STATE_PRELIGHT)
        return 2;
    if (state_type == GTK_STATE_ACTIVE)
        return 3;
    return 1;
}

static void msstyles_draw_arrow(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, GtkArrowType arrow_type, gboolean fill, gint x, gint y, gint width, gint height) {
    MsStylesStyle *msstyles_style = MSSTYLES_STYLE(style);
    gint state_index;
    gboolean drew = FALSE;

    if (msstyles_style->theme_ini && detail) {
        state_index = msstyles_arrow_state(state_type);
        if (strcmp(detail, "spinbutton") == 0) {
            const gchar *part = (arrow_type == GTK_ARROW_UP) ? "UP" : "DOWN";
            drew = msstyles_render_part(msstyles_style, window, area, "SPIN", part, state_index, x, y, width, height);
        } else if (strcmp(detail, "vscrollbar") == 0 || strcmp(detail, "hscrollbar") == 0) {
            const gchar *part;
            if (arrow_type == GTK_ARROW_UP)
                part = "ARROWBTN";
            else if (arrow_type == GTK_ARROW_DOWN)
                part = "ARROWBTN";
            else if (arrow_type == GTK_ARROW_LEFT)
                part = "ARROWBTN";
            else
                part = "ARROWBTN";
            drew = msstyles_render_part(msstyles_style, window, area, "SCROLLBAR", part, state_index, x, y, width, height);
        } else if (strcmp(detail, "optionmenu") == 0 || strcmp(detail, "combobox") == 0) {
            drew = msstyles_render_part(msstyles_style, window, area, "COMBOBOX", "DROPDOWNBUTTON", state_index, x, y, width, height);
        }
    }

    if (!drew)
        parent_class->draw_arrow(style, window, state_type, shadow_type, area, widget, detail, arrow_type, fill, x, y, width, height);
}

static void msstyles_draw_box(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height) {
    MsStylesStyle *msstyles_style = MSSTYLES_STYLE(style);
    gint state_index;

    if (msstyles_style->theme_ini && detail) {
        if (strcmp(detail, "button") == 0 || strcmp(detail, "buttondefault") == 0) {
            state_index = msstyles_pushbutton_state(state_type, shadow_type);
            if (msstyles_render_part(msstyles_style, window, area, "BUTTON", "PUSHBUTTON", state_index, x, y, width, height))
                return;
        } else if (strcmp(detail, "entry") == 0 || strcmp(detail, "combobox") == 0) {
            state_index = state_type == GTK_STATE_INSENSITIVE ? 4 : (state_type == GTK_STATE_ACTIVE ? 3 : 1);
            if (msstyles_render_part(msstyles_style, window, area, "EDIT", "EDITTEXT", state_index, x, y, width, height))
                return;
        } else if (strcmp(detail, "trough") == 0) {
            const gchar *part = (width > height) ? "TICKS" : "TICKS";
            if (msstyles_render_part(msstyles_style, window, area, "PROGRESS", "TRANSPARENTBAR", 1, x, y, width, height))
                return;
            if (msstyles_render_part(msstyles_style, window, area, "TRACKBAR", "TRACK", 1, x, y, width, height))
                return;
        } else if (strncmp(detail, "spinbutton", 10) == 0) {
            state_index = msstyles_pushbutton_state(state_type, shadow_type);
            if (msstyles_render_part(msstyles_style, window, area, "SPIN", "UPHORZ", state_index, x, y, width, height))
                return;
        } else if (strcmp(detail, "menu") == 0) {
            if (msstyles_render_part(msstyles_style, window, area, "MENU", "POPUPBORDERS", 1, x, y, width, height))
                return;
        } else if (strcmp(detail, "toolbar") == 0 || strcmp(detail, "handlebox_bin") == 0 || strcmp(detail, "dockitem_bin") == 0) {
            if (msstyles_render_part(msstyles_style, window, area, "REBAR", "BAND", 1, x, y, width, height))
                return;
        } else if (strcmp(detail, "menubar") == 0) {
            if (msstyles_render_part(msstyles_style, window, area, "MENU", "MENUBAR", 1, x, y, width, height))
                return;
        } else if (strcmp(detail, "menuitem") == 0) {
            state_index = state_type == GTK_STATE_PRELIGHT ? 2 : 1;
            if (msstyles_render_part(msstyles_style, window, area, "MENU", "MENUITEM", state_index, x, y, width, height))
                return;
        } else if (strcmp(detail, "hscrollbar") == 0 || strcmp(detail, "vscrollbar") == 0) {
            if (msstyles_render_part(msstyles_style, window, area, "SCROLLBAR", "ARROWBTN", 1, x, y, width, height))
                return;
        } else if (strcmp(detail, "stepper") == 0) {
            state_index = msstyles_pushbutton_state(state_type, shadow_type);
            if (msstyles_render_part(msstyles_style, window, area, "SCROLLBAR", "ARROWBTN", state_index, x, y, width, height))
                return;
        } else if (strcmp(detail, "slider") == 0) {
            state_index = msstyles_thumb_state(state_type);
            if (msstyles_render_part(msstyles_style, window, area, "SCROLLBAR", "THUMB", state_index, x, y, width, height))
                return;
        } else if (strcmp(detail, "bar") == 0) {
            if (msstyles_render_part(msstyles_style, window, area, "PROGRESS", "BAR", 1, x, y, width, height))
                return;
        } else if (strcmp(detail, "trough-upper") == 0 || strcmp(detail, "trough-lower") == 0) {
            if (msstyles_render_part(msstyles_style, window, area, "SCROLLBAR", "TRACK", 1, x, y, width, height))
                return;
        } else if (strcmp(detail, "optionmenu") == 0) {
            state_index = msstyles_pushbutton_state(state_type, shadow_type);
            if (msstyles_render_part(msstyles_style, window, area, "COMBOBOX", "DROPDOWNBUTTON", state_index, x, y, width, height))
                return;
        } else if (strcmp(detail, "notebook") == 0) {
            if (msstyles_render_part(msstyles_style, window, area, "TAB", "PANE", 1, x, y, width, height))
                return;
        } else if (strcmp(detail, "paned") == 0) {
            if (msstyles_render_part(msstyles_style, window, area, "REBAR", "GRIPPER", 1, x, y, width, height))
                return;
        }
    }

    parent_class->draw_box(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
}

static void msstyles_draw_flat_box(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height) {
    MsStylesStyle *msstyles_style = MSSTYLES_STYLE(style);
    cairo_t *cr;
    GdkColor *color;
    double r, g, b;
    gboolean drew = FALSE;

    if (!msstyles_style->theme_ini) {
        parent_class->draw_flat_box(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
        return;
    }

    if (detail) {
        if (strcmp(detail, "menu") == 0) {
            drew = msstyles_render_part(msstyles_style, window, area, "MENU", "POPUPBACKGROUND", 1, x, y, width, height);
        } else if (strcmp(detail, "menuitem") == 0) {
            gint state_index = state_type == GTK_STATE_PRELIGHT ? 2 : 1;
            drew = msstyles_render_part(msstyles_style, window, area, "MENU", "POPUPITEM", state_index, x, y, width, height);
        } else if (strcmp(detail, "menubar") == 0) {
            drew = msstyles_render_part(msstyles_style, window, area, "MENU", "BARBACKGROUND", 1, x, y, width, height);
        } else if (strcmp(detail, "tooltip") == 0) {
            drew = msstyles_render_part(msstyles_style, window, area, "TOOLTIP", "STANDARD", 1, x, y, width, height);
        } else if (strcmp(detail, "base") == 0 || strcmp(detail, "bg") == 0) {
            drew = msstyles_render_part(msstyles_style, window, area, "WINDOW", "DIALOG", 1, x, y, width, height);
        }
    } else {
        drew = msstyles_render_part(msstyles_style, window, area, "WINDOW", "DIALOG", 1, x, y, width, height);
    }

    if (drew)
        return;

    color = detail && strcmp(detail, "base") == 0 ? &style->base[state_type] : &style->bg[state_type];
    r = color->red / 65535.0;
    g = color->green / 65535.0;
    b = color->blue / 65535.0;

    cr = gdk_cairo_create(window);

    if (area) {
        gdk_cairo_rectangle(cr, area);
        cairo_clip(cr);
    }

    cairo_set_source_rgb(cr, r, g, b);
    cairo_rectangle(cr, x, y, width, height);
    cairo_fill(cr);
    cairo_destroy(cr);
}

static void msstyles_draw_shadow(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height) {
    MsStylesStyle *msstyles_style = MSSTYLES_STYLE(style);
    gint state_index;
    gboolean drew = FALSE;

    if (msstyles_style->theme_ini && detail && shadow_type != GTK_SHADOW_NONE) {
        if (strcmp(detail, "entry") == 0 || strcmp(detail, "combobox") == 0) {
            state_index = state_type == GTK_STATE_INSENSITIVE ? 4 : (state_type == GTK_STATE_ACTIVE ? 3 : 1);
            drew = msstyles_render_part(msstyles_style, window, area, "EDIT", "EDITTEXT", state_index, x, y, width, height);
        } else if (strcmp(detail, "frame") == 0) {
            drew = msstyles_render_part(msstyles_style, window, area, "LISTVIEW", "LISTVIEWITEM", 1, x, y, width, height);
        } else if (strcmp(detail, "toolbar") == 0 || strcmp(detail, "handlebox") == 0) {
            drew = msstyles_render_part(msstyles_style, window, area, "REBAR", "BAND", 1, x, y, width, height);
        } else if (strcmp(detail, "menu") == 0) {
            drew = msstyles_render_part(msstyles_style, window, area, "MENU", "POPUPBORDERS", 1, x, y, width, height);
        } else if (strcmp(detail, "scrolled_window") == 0) {
            drew = msstyles_render_part(msstyles_style, window, area, "LISTVIEW", "LISTVIEWITEM", 1, x, y, width, height);
        }
    }

    if (!drew)
        parent_class->draw_shadow(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
}

static void msstyles_draw_check(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height) {
    MsStylesStyle *msstyles_style = MSSTYLES_STYLE(style);
    gint state_index;
    gboolean checked;

    if (!msstyles_style->theme_ini) {
        parent_class->draw_check(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
        return;
    }

    checked = shadow_type == GTK_SHADOW_IN || shadow_type == GTK_SHADOW_ETCHED_IN;
    state_index = msstyles_checkmark_state(state_type, checked);

    if (!msstyles_render_part(msstyles_style, window, area, "BUTTON", "CHECKBOX", state_index, x, y, width, height))
        parent_class->draw_check(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
}

static void msstyles_draw_option(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height) {
    MsStylesStyle *msstyles_style = MSSTYLES_STYLE(style);
    gint state_index;
    gboolean checked;

    if (!msstyles_style->theme_ini) {
        parent_class->draw_option(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
        return;
    }

    checked = shadow_type == GTK_SHADOW_IN || shadow_type == GTK_SHADOW_ETCHED_IN;
    state_index = msstyles_checkmark_state(state_type, checked);

    if (!msstyles_render_part(msstyles_style, window, area, "BUTTON", "RADIOBUTTON", state_index, x, y, width, height))
        parent_class->draw_option(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
}

static void msstyles_draw_tab(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height) {
    MsStylesStyle *msstyles_style = MSSTYLES_STYLE(style);
    gint state_index;

    if (!msstyles_style->theme_ini) {
        parent_class->draw_tab(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
        return;
    }

    state_index = msstyles_tabitem_state(state_type, shadow_type == GTK_SHADOW_OUT);

    if (!msstyles_render_part(msstyles_style, window, area, "TAB", "TABITEM", state_index, x, y, width, height))
        parent_class->draw_tab(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height);
}

static void msstyles_draw_slider(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height, GtkOrientation orientation) {
    MsStylesStyle *msstyles_style = MSSTYLES_STYLE(style);
    gint state_index;
    const gchar *part_name;

    if (!msstyles_style->theme_ini) {
        parent_class->draw_slider(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height, orientation);
        return;
    }

    state_index = msstyles_thumb_state(state_type);
    part_name = orientation == GTK_ORIENTATION_HORIZONTAL ? "THUMB" : "THUMBVERT";

    if (!msstyles_render_part(msstyles_style, window, area, "TRACKBAR", part_name, state_index, x, y, width, height))
        parent_class->draw_slider(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height, orientation);
}

static void msstyles_draw_handle(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height, GtkOrientation orientation) {
    MsStylesStyle *msstyles_style = MSSTYLES_STYLE(style);
    const gchar *part_name;

    if (!msstyles_style->theme_ini) {
        parent_class->draw_handle(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height, orientation);
        return;
    }

    part_name = orientation == GTK_ORIENTATION_HORIZONTAL ? "GRIPPER" : "GRIPPERVERT";

    if (!msstyles_render_part(msstyles_style, window, area, "REBAR", part_name, 1, x, y, width, height))
        parent_class->draw_handle(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height, orientation);
}

static void msstyles_draw_box_gap(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height, GtkPositionType gap_side, gint gap_x, gint gap_width) {
    MsStylesStyle *msstyles_style = MSSTYLES_STYLE(style);

    if (msstyles_style->theme_ini && detail && strcmp(detail, "notebook") == 0) {
        if (msstyles_render_part(msstyles_style, window, area, "TAB", "PANE", 1, x, y, width, height))
            return;
    }

    parent_class->draw_box_gap(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height, gap_side, gap_x, gap_width);
}

static void msstyles_draw_extension(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GtkShadowType shadow_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height, GtkPositionType gap_side) {
    MsStylesStyle *msstyles_style = MSSTYLES_STYLE(style);
    gint state_index;

    const gchar *class_name = "TAB";
    const gchar *part_name = "TABITEM";

    if (!msstyles_style->theme_ini) {
        parent_class->draw_extension(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height, gap_side);
        return;
    }

    if (detail && strcmp(detail, "tab") == 0) {
        class_name = "TAB";
        part_name = "TABITEM";
        state_index = msstyles_tabitem_state(state_type, shadow_type == GTK_SHADOW_OUT);
    } else {
        class_name = "HEADER";
        part_name = "HEADERITEM";
        state_index = state_type == GTK_STATE_PRELIGHT ? 2 : 1;
    }

    if (!msstyles_render_part(msstyles_style, window, area, class_name, part_name, state_index, x, y, width, height))
        parent_class->draw_extension(style, window, state_type, shadow_type, area, widget, detail, x, y, width, height, gap_side);
}

static void msstyles_draw_hline(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x1, gint x2, gint y) {
    MsStylesStyle *msstyles_style = MSSTYLES_STYLE(style);
    cairo_t *cr;
    GdkColor *color;
    double r, g, b;

    if (!msstyles_style->theme_ini) {
        parent_class->draw_hline(style, window, state_type, area, widget, detail, x1, x2, y);
        return;
    }

    color = &style->dark[state_type];
    r = color->red / 65535.0;
    g = color->green / 65535.0;
    b = color->blue / 65535.0;

    cr = gdk_cairo_create(window);

    if (area) {
        gdk_cairo_rectangle(cr, area);
        cairo_clip(cr);
    }

    cairo_set_source_rgb(cr, r, g, b);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, x1 + 0.5, y + 0.5);
    cairo_line_to(cr, x2 + 0.5, y + 0.5);
    cairo_stroke(cr);
    cairo_destroy(cr);
}

static void msstyles_draw_vline(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y1, gint y2) {
    MsStylesStyle *msstyles_style = MSSTYLES_STYLE(style);
    cairo_t *cr;
    GdkColor *color;
    double r, g, b;

    if (!msstyles_style->theme_ini) {
        parent_class->draw_vline(style, window, state_type, area, widget, detail, x, y1, y2);
        return;
    }

    color = &style->dark[state_type];
    r = color->red / 65535.0;
    g = color->green / 65535.0;
    b = color->blue / 65535.0;

    cr = gdk_cairo_create(window);

    if (area) {
        gdk_cairo_rectangle(cr, area);
        cairo_clip(cr);
    }

    cairo_set_source_rgb(cr, r, g, b);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, x + 0.5, y1 + 0.5);
    cairo_line_to(cr, x + 0.5, y2 + 0.5);
    cairo_stroke(cr);
    cairo_destroy(cr);
}

static void msstyles_draw_expander(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, GtkExpanderStyle expander_style) {
    MsStylesStyle *msstyles_style = MSSTYLES_STYLE(style);
    gint size = 9;
    gint state_index;
    gboolean collapsed;

    gtk_widget_style_get(widget, "expander-size", &size, NULL);
    if (size % 2 == 0)
        size--;
    if (size > 2)
        size -= 2;

    collapsed = (expander_style == GTK_EXPANDER_COLLAPSED || expander_style == GTK_EXPANDER_SEMI_COLLAPSED);
    state_index = collapsed ? 1 : 2;

    if (!msstyles_style->theme_ini || !msstyles_render_part(msstyles_style, window, area, "TREEVIEW", "GLYPH", state_index, x - size / 2, y - size / 2, size, size))
        parent_class->draw_expander(style, window, state_type, area, widget, detail, x, y, expander_style);
}

static void msstyles_draw_resize_grip(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, GdkWindowEdge edge, gint x, gint y, gint width, gint height) {
    MsStylesStyle *msstyles_style = MSSTYLES_STYLE(style);

    if (!msstyles_style->theme_ini || !msstyles_render_part(msstyles_style, window, area, "STATUS", "GRIPPER", 1, x, y, width, height))
        parent_class->draw_resize_grip(style, window, state_type, area, widget, detail, edge, x, y, width, height);
}

static void msstyles_draw_layout(GtkStyle *style, GdkWindow *window, GtkStateType state_type, gboolean use_text, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, PangoLayout *layout) {
    gint lx = x;
    gint ly = y;

    if (detail && strcmp(detail, "label") == 0 && widget && widget->parent && GTK_IS_NOTEBOOK(widget->parent)) {
        GtkNotebook *nb = GTK_NOTEBOOK(widget->parent);
        gint side = gtk_notebook_get_tab_pos(nb);
        if (side == GTK_POS_TOP || side == GTK_POS_BOTTOM)
            lx += 2;
    }

    parent_class->draw_layout(style, window, state_type, use_text, area, widget, detail, lx, ly, layout);
}

static void msstyles_draw_focus(GtkStyle *style, GdkWindow *window, GtkStateType state_type, GdkRectangle *area, GtkWidget *widget, const gchar *detail, gint x, gint y, gint width, gint height) {
    MsStylesStyle *msstyles_style = MSSTYLES_STYLE(style);
    cairo_t *cr;
    GdkColor *color;
    double r, g, b;

    if (!msstyles_style->theme_ini) {
        parent_class->draw_focus(style, window, state_type, area, widget, detail, x, y, width, height);
        return;
    }

    color = &style->text[GTK_STATE_SELECTED];
    r = color->red / 65535.0;
    g = color->green / 65535.0;
    b = color->blue / 65535.0;

    cr = gdk_cairo_create(window);

    if (area) {
        gdk_cairo_rectangle(cr, area);
        cairo_clip(cr);
    }

    cairo_set_source_rgb(cr, r, g, b);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, x + 0.5, y + 0.5, width - 1, height - 1);
    cairo_stroke(cr);
    cairo_destroy(cr);
}

static void msstyles_style_class_init(MsStylesStyleClass *klass) {
    GtkStyleClass *style_class = GTK_STYLE_CLASS(klass);
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = msstyles_style_finalize;
    style_class->init_from_rc = msstyles_style_init_from_rc;
    style_class->draw_box = msstyles_draw_box;
    style_class->draw_flat_box = msstyles_draw_flat_box;
    style_class->draw_shadow = msstyles_draw_shadow;
    style_class->draw_check = msstyles_draw_check;
    style_class->draw_option = msstyles_draw_option;
    style_class->draw_tab = msstyles_draw_tab;
    style_class->draw_slider = msstyles_draw_slider;
    style_class->draw_handle = msstyles_draw_handle;
    style_class->draw_box_gap = msstyles_draw_box_gap;
    style_class->draw_extension = msstyles_draw_extension;
    style_class->draw_hline = msstyles_draw_hline;
    style_class->draw_vline = msstyles_draw_vline;
    style_class->draw_focus = msstyles_draw_focus;
    style_class->draw_arrow = msstyles_draw_arrow;
    style_class->draw_expander = msstyles_draw_expander;
    style_class->draw_resize_grip = msstyles_draw_resize_grip;
    style_class->draw_layout = msstyles_draw_layout;
}
