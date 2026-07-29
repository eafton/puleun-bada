#include <gtk/gtkrc.h>

typedef struct _MsStylesRcStyle MsStylesRcStyle;
typedef struct _MsStylesRcStyleClass MsStylesRcStyleClass;

extern GType msstyles_type_rc_style;

#define MSSTYLES_TYPE_RC_STYLE msstyles_type_rc_style
#define MSSTYLES_RC_STYLE(object) (G_TYPE_CHECK_INSTANCE_CAST((object), MSSTYLES_TYPE_RC_STYLE, MsStylesRcStyle))
#define MSSTYLES_RC_STYLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), MSSTYLES_TYPE_RC_STYLE, MsStylesRcStyleClass))
#define MSSTYLES_IS_RC_STYLE(object) (G_TYPE_CHECK_INSTANCE_TYPE((object), MSSTYLES_TYPE_RC_STYLE))
#define MSSTYLES_IS_RC_STYLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MSSTYLES_TYPE_RC_STYLE))
#define MSSTYLES_RC_STYLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), MSSTYLES_TYPE_RC_STYLE, MsStylesRcStyleClass))

struct _MsStylesRcStyle {
    GtkRcStyle parent_instance;

    gchar *msstyles_path;
    gchar *color_scheme;
    gchar *font_size;
};

struct _MsStylesRcStyleClass {
    GtkRcStyleClass parent_class;
};

void msstyles_rc_style_register_type(GTypeModule *module);
