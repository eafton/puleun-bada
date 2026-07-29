#include "msstyles_rc_style.h"
#include "msstyles_style.h"

GtkRcStyleClass *parent_class;
GType msstyles_type_rc_style = 0;

static void msstyles_rc_style_init(MsStylesRcStyle *style);
static void msstyles_rc_style_class_init(MsStylesRcStyleClass *klass);
static void msstyles_rc_style_finalize(GObject *object);
static GtkStyle *msstyles_rc_style_create_style(GtkRcStyle *rc_style);
static guint msstyles_rc_style_parse(GtkRcStyle *rc_style, GtkSettings *settings, GScanner *scanner);
static void msstyles_rc_style_merge(GtkRcStyle *dest, GtkRcStyle *src);

enum {
    TOKEN_MSSTYLES = G_TOKEN_LAST + 1,
    TOKEN_COLOR_SCHEME,
    TOKEN_FONT_SIZE
};

struct {
    const gchar *name;
    guint token;
} theme_symbols[] = {{"msstyles", TOKEN_MSSTYLES}, {"color_scheme", TOKEN_COLOR_SCHEME}, {"font_size", TOKEN_FONT_SIZE}};

void msstyles_rc_style_register_type(GTypeModule *module) {
    GTypeInfo object_info;

    object_info.class_size = sizeof(MsStylesRcStyleClass);
    object_info.base_init = NULL;
    object_info.base_finalize = NULL;
    object_info.class_init = (GClassInitFunc)msstyles_rc_style_class_init;
    object_info.class_finalize = NULL;
    object_info.class_data = NULL;
    object_info.instance_size = sizeof(MsStylesRcStyle);
    object_info.n_preallocs = 0;
    object_info.instance_init = (GInstanceInitFunc)msstyles_rc_style_init;
    object_info.value_table = NULL;

    msstyles_type_rc_style = g_type_module_register_type(module, GTK_TYPE_RC_STYLE, "MsStylesRcStyle", &object_info, 0);
}

void msstyles_rc_style_init(MsStylesRcStyle *msstyles_rc) {
    msstyles_rc->msstyles_path = NULL;
    msstyles_rc->color_scheme = NULL;
    msstyles_rc->font_size = NULL;
}

void msstyles_rc_style_class_init(MsStylesRcStyleClass *klass) {
    GtkRcStyleClass *rc_style_class = GTK_RC_STYLE_CLASS(klass);
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = msstyles_rc_style_finalize;
    rc_style_class->parse = msstyles_rc_style_parse;
    rc_style_class->create_style = msstyles_rc_style_create_style;
    rc_style_class->merge = msstyles_rc_style_merge;
}

void msstyles_rc_style_finalize(GObject *object) {
    MsStylesRcStyle *msstyles_rc;

    msstyles_rc = MSSTYLES_RC_STYLE(object);

    if (msstyles_rc->msstyles_path) {
        g_free(msstyles_rc->msstyles_path);
        msstyles_rc->msstyles_path = NULL;
    }

    if (msstyles_rc->color_scheme) {
        g_free(msstyles_rc->color_scheme);
        msstyles_rc->color_scheme = NULL;
    }

    if (msstyles_rc->font_size) {
        g_free(msstyles_rc->font_size);
        msstyles_rc->font_size = NULL;
    }

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

guint theme_parse_string_value(GScanner *scanner, gchar **value) {
    guint token;

    token = g_scanner_get_next_token(scanner);

    if (token != G_TOKEN_EQUAL_SIGN)
        return G_TOKEN_EQUAL_SIGN;

    token = g_scanner_get_next_token(scanner);
    if (token != G_TOKEN_STRING)
        return G_TOKEN_STRING;

    *value = g_strdup(scanner->value.v_string);

    return G_TOKEN_NONE;
}

guint msstyles_rc_style_parse(GtkRcStyle *rc_style, GtkSettings *settings, GScanner *scanner) {
    MsStylesRcStyle *msstyles_style;
    GQuark scope_id;
    guint old_scope;
    guint token;
    guint i;
    guint actual_token;

    scope_id = 0;
    msstyles_style = MSSTYLES_RC_STYLE(rc_style);

    if (!scope_id)
        scope_id = g_quark_from_string("msstyles_theme_engine");

    old_scope = g_scanner_set_scope(scanner, scope_id);

    if (!g_scanner_lookup_symbol(scanner, theme_symbols[0].name)) {
        g_scanner_freeze_symbol_table(scanner);
        for (i = 0; i < G_N_ELEMENTS(theme_symbols); i++)
            g_scanner_scope_add_symbol(scanner, scope_id, theme_symbols[i].name, GINT_TO_POINTER(theme_symbols[i].token));
        g_scanner_thaw_symbol_table(scanner);
    }

    token = g_scanner_peek_next_token(scanner);
    while (token != G_TOKEN_RIGHT_CURLY) {
        actual_token = token;
        if (token == G_TOKEN_SYMBOL)
            actual_token = GPOINTER_TO_INT(scanner->value.v_symbol);

        if (actual_token == TOKEN_MSSTYLES) {
            g_scanner_get_next_token(scanner);
            token = theme_parse_string_value(scanner, &msstyles_style->msstyles_path);
        } else if (actual_token == TOKEN_COLOR_SCHEME) {
            g_scanner_get_next_token(scanner);
            token = theme_parse_string_value(scanner, &msstyles_style->color_scheme);
        } else if (actual_token == TOKEN_FONT_SIZE) {
            g_scanner_get_next_token(scanner);
            token = theme_parse_string_value(scanner, &msstyles_style->font_size);
        } else {
            g_scanner_get_next_token(scanner);
            token = G_TOKEN_RIGHT_CURLY;
        }

        if (token != G_TOKEN_NONE)
            return token;

        token = g_scanner_peek_next_token(scanner);
    }

    g_scanner_get_next_token(scanner);

    g_scanner_set_scope(scanner, old_scope);

    return G_TOKEN_NONE;
}

void msstyles_rc_style_merge(GtkRcStyle *dest, GtkRcStyle *src) {
    MsStylesRcStyle *dest_w, *src_w;

    parent_class->merge(dest, src);

    if (!MSSTYLES_IS_RC_STYLE(src))
        return;

    src_w = MSSTYLES_RC_STYLE(src);
    dest_w = MSSTYLES_RC_STYLE(dest);

    if (src_w->msstyles_path) {
        if (dest_w->msstyles_path)
            g_free(dest_w->msstyles_path);
        dest_w->msstyles_path = g_strdup(src_w->msstyles_path);
    }

    if (src_w->color_scheme) {
        if (dest_w->color_scheme)
            g_free(dest_w->color_scheme);
        dest_w->color_scheme = g_strdup(src_w->color_scheme);
    }

    if (src_w->font_size) {
        if (dest_w->font_size)
            g_free(dest_w->font_size);
        dest_w->font_size = g_strdup(src_w->font_size);
    }
}

GtkStyle *msstyles_rc_style_create_style(GtkRcStyle *rc_style) {
    MsStylesStyle *style;
    MsStylesRcStyle *msstyles_rc;

    msstyles_rc = MSSTYLES_RC_STYLE(rc_style);

    style = g_object_new(MSSTYLES_TYPE_STYLE, NULL);

    if (msstyles_rc->msstyles_path)
        style->msstyles_path = g_strdup(msstyles_rc->msstyles_path);

    if (msstyles_rc->color_scheme)
        style->color_scheme = g_strdup(msstyles_rc->color_scheme);

    if (msstyles_rc->font_size)
        style->font_size = g_strdup(msstyles_rc->font_size);

    return GTK_STYLE(style);
}
