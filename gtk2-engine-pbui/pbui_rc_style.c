#include "pbui.h"
#include "pbui_rc_style.h"
#include "pbui_style.h"

GtkRcStyleClass *parent_class;
GType pbui_type_rc_style = 0;

static void pbui_rc_style_init(PbuiRcStyle *style);
static void pbui_rc_style_class_init(PbuiRcStyleClass *klass);
static void pbui_rc_style_finalize(GObject *object);
static GtkStyle *pbui_rc_style_create_style(GtkRcStyle *rc_style);
static guint pbui_rc_style_parse(GtkRcStyle *rc_style, GtkSettings *settings, GScanner *scanner);
static void pbui_rc_style_merge(GtkRcStyle *dest, GtkRcStyle *src);

enum {
    TOKEN_GTK3_THEME = G_TOKEN_LAST + 1,
    TOKEN_INTEGER_SCALE
};

struct {
    const gchar *name;
    guint token;
} theme_symbols[] = {{"gtk3_theme", TOKEN_GTK3_THEME}, {"integer_scale", TOKEN_INTEGER_SCALE}};

void pbui_rc_style_register_type(GTypeModule *module) {
    GTypeInfo object_info;

    object_info.class_size = sizeof(PbuiRcStyleClass);
    object_info.base_init = NULL;
    object_info.base_finalize = NULL;
    object_info.class_init = (GClassInitFunc)pbui_rc_style_class_init;
    object_info.class_finalize = NULL;
    object_info.class_data = NULL;
    object_info.instance_size = sizeof(PbuiRcStyle);
    object_info.n_preallocs = 0;
    object_info.instance_init = (GInstanceInitFunc)pbui_rc_style_init;
    object_info.value_table = NULL;

    pbui_type_rc_style = g_type_module_register_type(module, GTK_TYPE_RC_STYLE, "PbuiRcStyle", &object_info, 0);
}

void pbui_rc_style_init(PbuiRcStyle *pbui_rc) {
    pbui_rc->gtk3_theme_path = NULL;
    pbui_rc->integer_scale = 1;
}

void pbui_rc_style_class_init(PbuiRcStyleClass *klass) {
    GtkRcStyleClass *rc_style_class = GTK_RC_STYLE_CLASS(klass);
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    parent_class = g_type_class_peek_parent(klass);

    gobject_class->finalize = pbui_rc_style_finalize;
    rc_style_class->parse = pbui_rc_style_parse;
    rc_style_class->create_style = pbui_rc_style_create_style;
    rc_style_class->merge = pbui_rc_style_merge;
}

void pbui_rc_style_finalize(GObject *object) {
    PbuiRcStyle *pbui_rc;

    pbui_rc = PBUI_RC_STYLE(object);

    if (pbui_rc->gtk3_theme_path) {
        g_free(pbui_rc->gtk3_theme_path);
        pbui_rc->gtk3_theme_path = NULL;
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

guint theme_parse_int_value(GScanner *scanner, gint *value) {
    guint token;

    token = g_scanner_get_next_token(scanner);

    if (token != G_TOKEN_EQUAL_SIGN)
        return G_TOKEN_EQUAL_SIGN;

    token = g_scanner_get_next_token(scanner);
    if (token != G_TOKEN_INT)
        return G_TOKEN_INT;

    *value = scanner->value.v_int;

    return G_TOKEN_NONE;
}

guint pbui_rc_style_parse(GtkRcStyle *rc_style, GtkSettings *settings, GScanner *scanner) {
    PbuiRcStyle *pbui_style;
    GQuark scope_id;
    guint old_scope;
    guint token;
    guint i;
    guint actual_token;

    scope_id = 0;
    pbui_style = PBUI_RC_STYLE(rc_style);

    if (!scope_id)
        scope_id = g_quark_from_string("pbui_theme_engine");

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

        if (actual_token == TOKEN_GTK3_THEME) {
            g_scanner_get_next_token(scanner);
            token = theme_parse_string_value(scanner, &pbui_style->gtk3_theme_path);
        } else if (actual_token == TOKEN_INTEGER_SCALE) {
            g_scanner_get_next_token(scanner);
            token = theme_parse_int_value(scanner, &pbui_style->integer_scale);
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

void pbui_rc_style_merge(GtkRcStyle *dest, GtkRcStyle *src) {
    PbuiRcStyle *dest_w, *src_w;

    parent_class->merge(dest, src);

    if (!PBUI_IS_RC_STYLE(src))
        return;

    src_w = PBUI_RC_STYLE(src);
    dest_w = PBUI_RC_STYLE(dest);

    if (src_w->gtk3_theme_path) {
        if (dest_w->gtk3_theme_path)
            g_free(dest_w->gtk3_theme_path);
        dest_w->gtk3_theme_path = g_strdup(src_w->gtk3_theme_path);
    }

    dest_w->integer_scale = src_w->integer_scale;
}

GtkStyle *pbui_rc_style_create_style(GtkRcStyle *rc_style) {
    PbuiStyle *style;
    PbuiRcStyle *pbui_rc;

    pbui_rc = PBUI_RC_STYLE(rc_style);

    style = g_object_new(PBUI_TYPE_STYLE, NULL);

    if (pbui_rc->gtk3_theme_path)
        style->gtk3_theme_path = g_strdup(pbui_rc->gtk3_theme_path);

    style->integer_scale = pbui_rc->integer_scale;

    return GTK_STYLE(style);
}
