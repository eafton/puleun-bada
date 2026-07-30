#ifndef PBUI_CSS_PARSER_H
#define PBUI_CSS_PARSER_H

#include <glib.h>
#include <glib-object.h>
#include <cairo.h>

#include "pbui-gtk2-export.h"

G_BEGIN_DECLS

#define PBUI_TYPE_CSS_PARSER (pbui_css_parser_get_type())
#define PBUI_CSS_PARSER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), PBUI_TYPE_CSS_PARSER, PBUICSSParser))
#define PBUI_CSS_PARSER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), PBUI_TYPE_CSS_PARSER, PBUICSSParserClass))
#define PBUI_IS_CSS_PARSER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), PBUI_TYPE_CSS_PARSER))
#define PBUI_IS_CSS_PARSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PBUI_TYPE_CSS_PARSER))
#define PBUI_CSS_PARSER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), PBUI_TYPE_CSS_PARSER, PBUICSSParserClass))

typedef struct _PBUICSSParser PBUICSSParser;
typedef struct _PBUICSSParserClass PBUICSSParserClass;

typedef struct {
    gdouble red;
    gdouble green;
    gdouble blue;
    gdouble alpha;
} PBUIColor;

typedef struct {
    gchar *name;
    PBUIColor color;
} PBUIColorDefinition;

typedef enum {
    PBUI_GRADIENT_LINEAR,
    PBUI_GRADIENT_NONE
} PBUIGradientType;

typedef struct {
    PBUIGradientType type;
    PBUIColor *colors;
    gint n_colors;
} PBUIGradient;

typedef struct {
    gchar *selector;
    PBUIGradient *background;
    PBUIColor *border_color;
    gdouble border_width;
    gdouble radius;
} PBUICSSRule;

struct _PBUICSSParser
{
    GObject parent;

    GHashTable *color_defs;
    GHashTable *rules;
    gint integer_scale;
};

struct _PBUICSSParserClass
{
    GObjectClass parent_class;
};

PBUI_GTK2_EXPORT GType pbui_css_parser_get_type(void);
PBUI_GTK2_EXPORT PBUICSSParser* pbui_css_parser_new(void);
PBUI_GTK2_EXPORT gboolean pbui_css_parser_parse_file(PBUICSSParser* parser, const gchar* filename);
PBUI_GTK2_EXPORT void pbui_css_parser_set_integer_scale(PBUICSSParser* parser, gint scale);
PBUI_GTK2_EXPORT PBUIColor* pbui_css_parser_lookup_color(PBUICSSParser* parser, const gchar* name);
PBUI_GTK2_EXPORT PBUICSSRule* pbui_css_parser_lookup_rule(PBUICSSParser* parser, const gchar* selector);
PBUI_GTK2_EXPORT void pbui_css_parser_free_color(PBUIColor* color);
PBUI_GTK2_EXPORT void pbui_css_parser_free_gradient(PBUIGradient* gradient);
PBUI_GTK2_EXPORT void pbui_css_parser_free_rule(PBUICSSRule* rule);

G_END_DECLS

#endif
