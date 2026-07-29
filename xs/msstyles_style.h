#include <gtk/gtkstyle.h>

typedef struct _MsStylesStyle MsStylesStyle;
typedef struct _MsStylesStyleClass MsStylesStyleClass;

extern GType msstyles_type_style;

#define MSSTYLES_TYPE_STYLE msstyles_type_style
#define MSSTYLES_STYLE(object) (G_TYPE_CHECK_INSTANCE_CAST((object), MSSTYLES_TYPE_STYLE, MsStylesStyle))
#define MSSTYLES_STYLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), MSSTYLES_TYPE_STYLE, MsStylesStyleClass))
#define MSSTYLES_IS_STYLE(object) (G_TYPE_CHECK_INSTANCE_TYPE((object), MSSTYLES_TYPE_STYLE))
#define MSSTYLES_IS_STYLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MSSTYLES_TYPE_STYLE))
#define MSSTYLES_STYLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), MSSTYLES_TYPE_STYLE, MsStylesStyleClass))

struct _MsStylesStyle {
    GtkStyle parent_instance;

    gchar *msstyles_path;
    gchar *color_scheme;
    gchar *font_size;
    GKeyFile *theme_ini;
};

struct _MsStylesStyleClass {
    GtkStyleClass parent_class;
};

void msstyles_style_register_type(GTypeModule *module);
