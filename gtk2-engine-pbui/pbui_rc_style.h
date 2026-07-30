#include <gtk/gtkrc.h>

typedef struct _PbuiRcStyle PbuiRcStyle;
typedef struct _PbuiRcStyleClass PbuiRcStyleClass;

extern GType pbui_type_rc_style;

#define PBUI_TYPE_RC_STYLE pbui_type_rc_style
#define PBUI_RC_STYLE(object) (G_TYPE_CHECK_INSTANCE_CAST((object), PBUI_TYPE_RC_STYLE, PbuiRcStyle))
#define PBUI_RC_STYLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), PBUI_TYPE_RC_STYLE, PbuiRcStyleClass))
#define PBUI_IS_RC_STYLE(object) (G_TYPE_CHECK_INSTANCE_TYPE((object), PBUI_TYPE_RC_STYLE))
#define PBUI_IS_RC_STYLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PBUI_TYPE_RC_STYLE))
#define PBUI_RC_STYLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), PBUI_TYPE_RC_STYLE, PbuiRcStyleClass))

struct _PbuiRcStyle {
    GtkRcStyle parent_instance;

    gchar *gtk3_theme_path;
    gint integer_scale;
};

struct _PbuiRcStyleClass {
    GtkRcStyleClass parent_class;
};

void pbui_rc_style_register_type(GTypeModule *module);
