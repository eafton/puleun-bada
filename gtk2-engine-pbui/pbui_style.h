#include <gtk/gtkstyle.h>
#include "pbui-css-parser.h"

typedef struct _PbuiStyle PbuiStyle;
typedef struct _PbuiStyleClass PbuiStyleClass;

extern GType pbui_type_style;

#define PBUI_TYPE_STYLE pbui_type_style
#define PBUI_STYLE(object) (G_TYPE_CHECK_INSTANCE_CAST((object), PBUI_TYPE_STYLE, PbuiStyle))
#define PBUI_STYLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), PBUI_TYPE_STYLE, PbuiStyleClass))
#define PBUI_IS_STYLE(object) (G_TYPE_CHECK_INSTANCE_TYPE((object), PBUI_TYPE_STYLE))
#define PBUI_IS_STYLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PBUI_TYPE_STYLE))
#define PBUI_STYLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), PBUI_TYPE_STYLE, PbuiStyleClass))

struct _PbuiStyle {
    GtkStyle parent_instance;

    gchar *gtk3_theme_path;
    gint integer_scale;
    PBUICSSParser *parser;
};

struct _PbuiStyleClass {
    GtkStyleClass parent_class;
};

void pbui_style_register_type(GTypeModule *module);
