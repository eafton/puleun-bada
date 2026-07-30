#include "pbui-css-parser.h"
#include <glib.h>
#include <string.h>

G_DEFINE_TYPE(PBUICSSParser, pbui_css_parser, G_TYPE_OBJECT)

static void pbui_css_parser_finalize(GObject* object);
static PBUIColor* parse_color_value(PBUICSSParser* parser, const gchar* value);
static PBUIGradient* parse_gradient(PBUICSSParser* parser, const gchar* value);
static void parse_define_color(PBUICSSParser* parser, const gchar* line);
static void parse_css_rule(PBUICSSParser* parser, const gchar* selector, const gchar* content);

static void pbui_css_parser_class_init(PBUICSSParserClass* klass)
{
    GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = pbui_css_parser_finalize;
}

static void pbui_css_parser_init(PBUICSSParser* parser)
{
    parser->color_defs = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    parser->rules = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    parser->integer_scale = 1;
}

static void pbui_css_parser_finalize(GObject* object)
{
    PBUICSSParser* parser = PBUI_CSS_PARSER(object);

    if (parser->color_defs) {
        g_hash_table_destroy(parser->color_defs);
        parser->color_defs = NULL;
    }

    if (parser->rules) {
        GHashTableIter iter;
        gpointer key, value;
        g_hash_table_iter_init(&iter, parser->rules);
        while (g_hash_table_iter_next(&iter, &key, &value)) {
            pbui_css_parser_free_rule((PBUICSSRule*)value);
        }
        g_hash_table_destroy(parser->rules);
        parser->rules = NULL;
    }

    G_OBJECT_CLASS(pbui_css_parser_parent_class)->finalize(object);
}

PBUICSSParser* pbui_css_parser_new(void)
{
    return g_object_new(PBUI_TYPE_CSS_PARSER, NULL);
}

static PBUIColor* parse_color_value(PBUICSSParser* parser, const gchar* value)
{
    PBUIColor* color = g_new0(PBUIColor, 1);
    gchar* value_lower = g_ascii_strdown(value, -1);

    if (g_str_has_prefix(value_lower, "#")) {
        gint len = strlen(value_lower);
        if (len == 7) {
            color->red = g_ascii_xdigit_value(value_lower[1]) * 16 + g_ascii_xdigit_value(value_lower[2]);
            color->green = g_ascii_xdigit_value(value_lower[3]) * 16 + g_ascii_xdigit_value(value_lower[4]);
            color->blue = g_ascii_xdigit_value(value_lower[5]) * 16 + g_ascii_xdigit_value(value_lower[6]);
            color->alpha = 1.0;
        } else if (len == 4) {
            color->red = g_ascii_xdigit_value(value_lower[1]) * 17;
            color->green = g_ascii_xdigit_value(value_lower[2]) * 17;
            color->blue = g_ascii_xdigit_value(value_lower[3]) * 17;
            color->alpha = 1.0;
        }
        color->red /= 255.0;
        color->green /= 255.0;
        color->blue /= 255.0;
    } else if (g_str_has_prefix(value_lower, "rgb(")) {
        gchar** parts = g_strsplit(value_lower + 4, ",", 3);
        if (g_strv_length(parts) == 3) {
            color->red = g_ascii_strtod(parts[0], NULL) / 255.0;
            color->green = g_ascii_strtod(parts[1], NULL) / 255.0;
            color->blue = g_ascii_strtod(parts[2], NULL) / 255.0;
            color->alpha = 1.0;
        }
        g_strfreev(parts);
    } else if (g_str_has_prefix(value_lower, "shade(")) {
        gchar* paren = strchr(value_lower, '(');
        gchar* end = strrchr(value_lower, ')');
        if (paren && end) {
            gchar* inner = g_strndup(paren + 1, end - paren - 1);
            gchar** parts = g_strsplit(inner, ",", 2);
            if (g_strv_length(parts) == 2) {
                PBUIColor* base_color = parse_color_value(parser, parts[0]);
                gdouble factor = g_ascii_strtod(parts[1], NULL);
                if (base_color) {
                    color->red = CLAMP(base_color->red * factor, 0.0, 1.0);
                    color->green = CLAMP(base_color->green * factor, 0.0, 1.0);
                    color->blue = CLAMP(base_color->blue * factor, 0.0, 1.0);
                    color->alpha = base_color->alpha;
                    pbui_css_parser_free_color(base_color);
                }
            }
            g_strfreev(parts);
            g_free(inner);
        }
    } else if (g_str_has_prefix(value_lower, "alpha(")) {
        gchar* paren = strchr(value_lower, '(');
        gchar* end = strrchr(value_lower, ')');
        if (paren && end) {
            gchar* inner = g_strndup(paren + 1, end - paren - 1);
            gchar** parts = g_strsplit(inner, ",", 2);
            if (g_strv_length(parts) == 2) {
                PBUIColor* base_color = parse_color_value(parser, parts[0]);
                gdouble alpha = g_ascii_strtod(parts[1], NULL);
                if (base_color) {
                    *color = *base_color;
                    color->alpha = CLAMP(alpha, 0.0, 1.0);
                    pbui_css_parser_free_color(base_color);
                }
            }
            g_strfreev(parts);
            g_free(inner);
        }
    } else {
        PBUIColor* def = pbui_css_parser_lookup_color(parser, value);
        if (def) {
            *color = *def;
        }
    }

    g_free(value_lower);
    return color;
}

static PBUIGradient* parse_gradient(PBUICSSParser* parser, const gchar* value)
{
    PBUIGradient* gradient = NULL;
    gchar* value_lower = g_ascii_strdown(value, -1);

    if (g_str_has_prefix(value_lower, "linear-gradient(")) {
        gradient = g_new0(PBUIGradient, 1);
        gradient->type = PBUI_GRADIENT_LINEAR;

        gchar* paren = strchr(value_lower, '(');
        gchar* end = strrchr(value_lower, ')');
        if (paren && end) {
            gchar* inner = g_strndup(paren + 1, end - paren - 1);
            gchar** stops = g_strsplit(inner, ",", 0);
            gint n_stops = g_strv_length(stops);

            if (n_stops > 0) {
                gint i;
                gradient->colors = g_new0(PBUIColor, n_stops);
                gradient->n_colors = 0;

                for (i = 0; i < n_stops; i++) {
                    gchar* stop = g_strstrip(stops[i]);
                    if (strlen(stop) > 0) {
                        PBUIColor* c = parse_color_value(parser, stop);
                        if (c) {
                            gradient->colors[gradient->n_colors++] = *c;
                            g_free(c);
                        }
                    }
                }
            }
            g_strfreev(stops);
            g_free(inner);
        }
    }

    g_free(value_lower);
    return gradient;
}

static void parse_define_color(PBUICSSParser* parser, const gchar* line)
{
    gchar* define = g_strstr_len(line, -1, "@define-color");
    if (define) {
        gchar* name_start = define + 13;
        while (*name_start == ' ') name_start++;

        gchar* space = strchr(name_start, ' ');
        if (space) {
            gchar* name = g_strndup(name_start, space - name_start);
            gchar* value = g_strstrip(g_strdup(space + 1));

            gchar* semicolon = strchr(value, ';');
            if (semicolon) {
                *semicolon = '\0';
            }

            PBUIColor* color = parse_color_value(parser, value);
            if (color) {
                g_hash_table_insert(parser->color_defs, name, color);
            } else {
                g_free(name);
            }
            g_free(value);
        }
    }
}

static void parse_css_rule(PBUICSSParser* parser, const gchar* selector, const gchar* content)
{
    PBUICSSRule* rule = g_new0(PBUICSSRule, 1);
    rule->selector = g_strdup(selector);
    rule->background = NULL;
    rule->border_color = NULL;
    rule->border_width = 0.0;
    rule->radius = 0.0;

    gchar** lines = g_strsplit(content, ";", 0);
    gint i;
    for (i = 0; lines[i]; i++) {
        gchar* line = g_strstrip(lines[i]);
        if (strlen(line) == 0) continue;

        gchar* colon = strchr(line, ':');
        if (colon) {
            gchar* prop = g_strstrip(g_strndup(line, colon - line));
            gchar* value = g_strstrip(g_strdup(colon + 1));

            if (g_ascii_strcasecmp(prop, "background-image") == 0 || 
                g_ascii_strcasecmp(prop, "background") == 0) {
                if (rule->background) {
                    pbui_css_parser_free_gradient(rule->background);
                }
                rule->background = parse_gradient(parser, value);
            } else if (g_ascii_strcasecmp(prop, "background-color") == 0) {
                if (!rule->background) {
                    PBUIColor* c = parse_color_value(parser, value);
                    if (c) {
                        rule->background = g_new0(PBUIGradient, 1);
                        rule->background->type = PBUI_GRADIENT_NONE;
                        rule->background->colors = c;
                        rule->background->n_colors = 1;
                    }
                }
            } else if (g_ascii_strcasecmp(prop, "border-color") == 0) {
                if (rule->border_color) {
                    pbui_css_parser_free_color(rule->border_color);
                }
                rule->border_color = parse_color_value(parser, value);
            } else if (g_ascii_strcasecmp(prop, "border-width") == 0) {
                rule->border_width = g_ascii_strtod(value, NULL);
            } else if (g_ascii_strcasecmp(prop, "border-radius") == 0) {
                rule->radius = g_ascii_strtod(value, NULL);
            }

            g_free(prop);
            g_free(value);
        }
    }
    g_strfreev(lines);

    gchar** selectors = g_strsplit(selector, ",", 0);
    for (i = 0; selectors[i]; i++) {
        gchar* sel = g_strstrip(selectors[i]);
        PBUICSSRule* rule_copy = g_new0(PBUICSSRule, 1);
        rule_copy->selector = g_strdup(sel);
        if (rule->background) {
            rule_copy->background = g_new0(PBUIGradient, 1);
            *rule_copy->background = *rule->background;
            rule_copy->background->colors = g_new0(PBUIColor, rule->background->n_colors);
            memcpy(rule_copy->background->colors, rule->background->colors, sizeof(PBUIColor) * rule->background->n_colors);
        }
        if (rule->border_color) {
            rule_copy->border_color = g_new0(PBUIColor, 1);
            *rule_copy->border_color = *rule->border_color;
        }
        rule_copy->border_width = rule->border_width;
        rule_copy->radius = rule->radius;
        g_hash_table_insert(parser->rules, g_strdup(sel), rule_copy);
        g_debug("[PBUI] Stored rule for selector: %s", sel);
    }
    g_strfreev(selectors);
    pbui_css_parser_free_rule(rule);
}

gboolean pbui_css_parser_parse_file(PBUICSSParser* parser, const gchar* filename)
{
    if (!parser || !filename) {
        return FALSE;
    }

    gchar* content = NULL;
    gsize length = 0;
    GError* error = NULL;

    if (!g_file_get_contents(filename, &content, &length, &error)) {
        if (error) {
            g_error_free(error);
        }
        return FALSE;
    }

    gchar** lines = g_strsplit(content, "\n", 0);
    g_free(content);

    gchar* current_selector = NULL;
    GString* current_content = NULL;
    gint i;

    for (i = 0; lines[i]; i++) {
        gchar* line = g_strstrip(lines[i]);

        if (strlen(line) == 0 || line[0] == '/' || line[0] == '*') {
            continue;
        }

        if (g_str_has_prefix(line, "@define-color")) {
            parse_define_color(parser, line);
        } else if (g_str_has_prefix(line, "@import")) {
            gchar* import_start = strchr(line, '"');
            if (import_start) {
                gchar* import_end = strchr(import_start + 1, '"');
                if (import_end) {
                    gchar* import_file = g_strndup(import_start + 1, import_end - import_start - 1);
                    gchar* import_path = g_build_filename(g_path_get_dirname(filename), import_file, NULL);
                    g_debug("[PBUI] Importing: %s", import_path);
                    pbui_css_parser_parse_file(parser, import_path);
                    g_free(import_path);
                    g_free(import_file);
                }
            }
        } else if (strchr(line, '{')) {
            gchar* brace = strchr(line, '{');
            if (current_selector) {
                g_free(current_selector);
            }
            current_selector = g_strstrip(g_strndup(line, brace - line));
            current_content = g_string_new("");
        } else if (strchr(line, '}')) {
            if (current_selector && current_content) {
                parse_css_rule(parser, current_selector, current_content->str);
            }
            if (current_selector) {
                g_free(current_selector);
                current_selector = NULL;
            }
            if (current_content) {
                g_string_free(current_content, TRUE);
                current_content = NULL;
            }
        } else if (current_content) {
            g_string_append(current_content, line);
            g_string_append_c(current_content, ';');
        }
    }

    if (current_selector) g_free(current_selector);
    if (current_content) g_string_free(current_content, TRUE);

    g_strfreev(lines);
    return TRUE;
}

void pbui_css_parser_set_integer_scale(PBUICSSParser* parser, gint scale)
{
    if (!parser) {
        return;
    }
    parser->integer_scale = scale;
}

PBUIColor* pbui_css_parser_lookup_color(PBUICSSParser* parser, const gchar* name)
{
    if (!parser || !name) {
        return NULL;
    }
    return g_hash_table_lookup(parser->color_defs, name);
}

PBUICSSRule* pbui_css_parser_lookup_rule(PBUICSSParser* parser, const gchar* selector)
{
    if (!parser || !selector) {
        return NULL;
    }
    return g_hash_table_lookup(parser->rules, selector);
}

void pbui_css_parser_free_color(PBUIColor* color)
{
    if (color) {
        g_free(color);
    }
}

void pbui_css_parser_free_gradient(PBUIGradient* gradient)
{
    if (gradient) {
        if (gradient->colors) {
            g_free(gradient->colors);
        }
        g_free(gradient);
    }
}

void pbui_css_parser_free_rule(PBUICSSRule* rule)
{
    if (rule) {
        if (rule->selector) g_free(rule->selector);
        if (rule->background) pbui_css_parser_free_gradient(rule->background);
        if (rule->border_color) pbui_css_parser_free_color(rule->border_color);
        g_free(rule);
    }
}
