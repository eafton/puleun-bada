#include <gtk/gtk.h>
#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <string.h>
#include <stdlib.h>

#define SILO_LABEL_MAX_CHARS 18

struct _PBSndVol;

typedef struct {
    struct _PBSndVol *app;
    guint32 index;
    gint type;
    GtkWidget *frame;
    GtkWidget *vol_scale;
    GtkWidget *mute_check;
    GtkWidget *bal_scale;
    pa_cvolume cv;
    gint updating;
    gint req_width;
} PBSndVolSiloData;

typedef struct {
    struct _PBSndVol *app;
    guint32 index;
    gint is_sink;
    gchar *port_name;
} PBSndVolPortData;

typedef struct _PBSndVol {
    pa_glib_mainloop *paloop;
    pa_mainloop_api *paapi;
    pa_context *pactx;
    GtkWidget *window;
    GtkWidget *master_box;
    GtkWidget *app_box;
    GtkWidget *hw_box;
    GtkWidget *app_empty;
    GtkWidget *hw_empty;
    GtkWidget *devices_menu;
    GList *silos;
    GList *port_items;
    gint menu_built;
    gint sink_count;
    gint no_devices_warned;
    gint ext_vol;
    GtkWidget *ext_vol_item;
    GtkStatusIcon *status_icon;
    GtkWidget *status_menu;
    GtkWidget *status_window;
    GtkWidget *status_vol_scale;
    GtkWidget *status_mute_check;
} PBSndVol;

void free_port_items(PBSndVol *app) {
    GList *l;
    PBSndVolPortData *pd;
    
    for (l = app->port_items; l; l = l->next) {
        pd = (PBSndVolPortData *)l->data;
        g_free(pd->port_name);
        g_free(pd);
    }
    g_list_free(app->port_items);
    app->port_items = NULL;
}

void clear_menu(GtkWidget *menu) {
    GList *children;
    GList *iter;
    
    children = gtk_container_get_children(GTK_CONTAINER(menu));
    for (iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
}

void update_empty_state(GtkWidget *box, GtkWidget *empty_label) {
    GList *children;
    gint has_silo;

    children = gtk_container_get_children(GTK_CONTAINER(box));
    has_silo = (g_list_length(children) > 1);
    g_list_free(children);

    if (has_silo) {
        gtk_widget_hide(empty_label);
    } else {
        gtk_widget_show(empty_label);
    }
}

PBSndVolSiloData *find_silo(PBSndVol *app, gint type, guint32 index) {
    GList *l;
    PBSndVolSiloData *sd;
    
    for (l = app->silos; l; l = l->next) {
        sd = (PBSndVolSiloData *)l->data;
        if (sd->type == type && sd->index == index) {
            return sd;
        }
    }
    return NULL;
}

void vol_changed(GtkRange *range, gpointer data) {
    PBSndVolSiloData *sd;
    gdouble val;
    pa_cvolume v;
    pa_operation *o;
    guint32 i;
    pa_volume_t new_vol;
    pa_volume_t max_vol;
    
    sd = (PBSndVolSiloData *)data;
    if (sd->updating) {
        return;
    }
    
    val = gtk_range_get_value(range);
    v = sd->cv;
    new_vol = (pa_volume_t)((val * PA_VOLUME_NORM) / 100.0);
    max_vol = pa_cvolume_max(&v);
    
    if (max_vol == 0) {
        pa_cvolume_set(&v, v.channels, new_vol);
    } else {
        for (i = 0; i < v.channels; i++) {
            v.values[i] = (pa_volume_t)(((double)v.values[i] / (double)max_vol) * (double)new_vol);
        }
    }
    
    o = NULL;
    if (sd->type == 0) {
        o = pa_context_set_sink_volume_by_index(sd->app->pactx, sd->index, &v, NULL, NULL);
    } else if (sd->type == 1) {
        o = pa_context_set_sink_input_volume(sd->app->pactx, sd->index, &v, NULL, NULL);
    } else if (sd->type == 2) {
        o = pa_context_set_source_volume_by_index(sd->app->pactx, sd->index, &v, NULL, NULL);
    }
    if (o) {
        pa_operation_unref(o);
    }
}

void mute_toggled(GtkToggleButton *btn, gpointer data) {
    PBSndVolSiloData *sd;
    gboolean m;
    pa_operation *o;
    
    sd = (PBSndVolSiloData *)data;
    if (sd->updating) {
        return;
    }
    
    m = gtk_toggle_button_get_active(btn);
    o = NULL;
    if (sd->type == 0) {
        o = pa_context_set_sink_mute_by_index(sd->app->pactx, sd->index, m, NULL, NULL);
    } else if (sd->type == 1) {
        o = pa_context_set_sink_input_mute(sd->app->pactx, sd->index, m, NULL, NULL);
    } else if (sd->type == 2) {
        o = pa_context_set_source_mute_by_index(sd->app->pactx, sd->index, m, NULL, NULL);
    }
    if (o) {
        pa_operation_unref(o);
    }
}

gboolean vol_scale_press_cb(GtkWidget *widget, GdkEventButton *ev, gpointer data) {
    if (ev->button == 1 && (ev->state & GDK_CONTROL_MASK)) {
        gtk_range_set_value(GTK_RANGE(widget), 50);
        return TRUE;
    }
    return FALSE;
}

gboolean bal_scale_press_cb(GtkWidget *widget, GdkEventButton *ev, gpointer data) {
    if (ev->button == 1 && (ev->state & GDK_CONTROL_MASK)) {
        gtk_range_set_value(GTK_RANGE(widget), 0);
        return TRUE;
    }
    return FALSE;
}

void bal_changed(GtkRange *range, gpointer data) {
    PBSndVolSiloData *sd;
    gdouble bal;
    pa_cvolume v;
    pa_operation *o;
    pa_volume_t max_v;
    
    sd = (PBSndVolSiloData *)data;
    if (sd->updating) {
        return;
    }
    if (sd->cv.channels != 2) {
        return;
    }
    
    bal = gtk_range_get_value(range);
    v = sd->cv;
    max_v = pa_cvolume_max(&v);
    
    if (bal < 0) {
        v.values[0] = max_v;
        v.values[1] = (pa_volume_t)(max_v * (1.0 + bal));
    } else {
        v.values[0] = (pa_volume_t)(max_v * (1.0 - bal));
        v.values[1] = max_v;
    }
    
    o = NULL;
    if (sd->type == 0) {
        o = pa_context_set_sink_volume_by_index(sd->app->pactx, sd->index, &v, NULL, NULL);
    } else if (sd->type == 1) {
        o = pa_context_set_sink_input_volume(sd->app->pactx, sd->index, &v, NULL, NULL);
    } else if (sd->type == 2) {
        o = pa_context_set_source_volume_by_index(sd->app->pactx, sd->index, &v, NULL, NULL);
    }
    if (o) {
        pa_operation_unref(o);
    }
}

void update_silo(PBSndVolSiloData *sd, const pa_cvolume *cv, gint mute) {
    gdouble v;
    gdouble bal;

    sd->updating = 1;
    sd->cv = *cv;
    v = (pa_cvolume_max(cv) * 100.0) / PA_VOLUME_NORM;
    gtk_range_set_value(GTK_RANGE(sd->vol_scale), v);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sd->mute_check), mute);
    
    if (cv->channels == 2) {
        if (cv->values[0] == 0 && cv->values[1] == 0) {
            bal = 0.0;
        } else if (cv->values[0] >= cv->values[1]) {
            bal = ((double)cv->values[1] / (double)cv->values[0]) - 1.0;
        } else {
            bal = 1.0 - ((double)cv->values[0] / (double)cv->values[1]);
        }
        gtk_range_set_value(GTK_RANGE(sd->bal_scale), bal);
    }
    sd->updating = 0;
}

void port_toggled_cb(GtkCheckMenuItem *item, gpointer data) {
    PBSndVolPortData *pd;
    pa_operation *o;

    pd = (PBSndVolPortData *)data;
    if (!gtk_check_menu_item_get_active(item)) {
        return;
    }
    
    o = NULL;
    if (pd->is_sink) {
        o = pa_context_set_sink_port_by_index(pd->app->pactx, pd->index, pd->port_name, NULL, NULL);
    } else {
        o = pa_context_set_source_port_by_index(pd->app->pactx, pd->index, pd->port_name, NULL, NULL);
    }
    if (o) {
        pa_operation_unref(o);
    }
}

void about_cb(GtkMenuItem *item, gpointer data) {
    GtkWidget *dialog;
    
    dialog = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), "Sound Mixer");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), "1.0");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), "A simple PulseAudio sound mixer");
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), "Copyright © 2026");
    gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dialog), "GPL");
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "https://example.com");
    
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void update_status_icon(PBSndVol *app) {
    PBSndVolSiloData *sd;
    gdouble vol;
    const gchar *icon_name;
    
    if (!app->silos) {
        return;
    }
    
    sd = (PBSndVolSiloData *)app->silos->data;
    if (sd->type != 0) {
        return;
    }
    
    if (sd->cv.channels == 0) {
        icon_name = "audio-volume-muted";
    } else {
        vol = (pa_cvolume_max(&sd->cv) * 100.0) / PA_VOLUME_NORM;
        
        if (vol == 0) {
            icon_name = "audio-volume-muted";
        } else if (vol < 33) {
            icon_name = "audio-volume-low";
        } else if (vol < 66) {
            icon_name = "audio-volume-medium";
        } else {
            icon_name = "audio-volume-high";
        }
    }
    
    gtk_status_icon_set_from_icon_name(app->status_icon, icon_name);
}

void status_vol_changed(GtkRange *range, gpointer data) {
    PBSndVol *app;
    gdouble val;
    pa_cvolume v;
    pa_operation *o;
    guint32 i;
    pa_volume_t new_vol;
    pa_volume_t max_vol;
    PBSndVolSiloData *sd;
    
    app = (PBSndVol *)data;
    
    if (!app->silos) {
        return;
    }
    
    sd = (PBSndVolSiloData *)app->silos->data;
    if (sd->type != 0) {
        return;
    }
    
    val = gtk_range_get_value(range);
    v = sd->cv;
    new_vol = (pa_volume_t)((val * PA_VOLUME_NORM) / 100.0);
    max_vol = pa_cvolume_max(&v);
    
    if (max_vol == 0) {
        pa_cvolume_set(&v, v.channels, new_vol);
    } else {
        for (i = 0; i < v.channels; i++) {
            v.values[i] = (pa_volume_t)(((double)v.values[i] / (double)max_vol) * (double)new_vol);
        }
    }
    
    o = pa_context_set_sink_volume_by_index(app->pactx, sd->index, &v, NULL, NULL);
    if (o) {
        pa_operation_unref(o);
    }
    
    update_status_icon(app);
}

void status_mute_toggled(GtkToggleButton *btn, gpointer data) {
    PBSndVol *app;
    gboolean m;
    pa_operation *o;
    PBSndVolSiloData *sd;
    
    app = (PBSndVol *)data;
    
    if (!app->silos) {
        return;
    }
    
    sd = (PBSndVolSiloData *)app->silos->data;
    if (sd->type != 0) {
        return;
    }
    
    m = gtk_toggle_button_get_active(btn);
    o = pa_context_set_sink_mute_by_index(app->pactx, sd->index, m, NULL, NULL);
    if (o) {
        pa_operation_unref(o);
    }
    
    update_status_icon(app);
}

void status_icon_popup(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer data) {
    PBSndVol *app = (PBSndVol *)data;
    
    gtk_menu_popup(GTK_MENU(app->status_menu), NULL, NULL, 
                   gtk_status_icon_position_menu, status_icon, 
                   button, activate_time);
}

void status_icon_clicked(GtkStatusIcon *status_icon, gpointer data) {
    PBSndVol *app = (PBSndVol *)data;
    gint x, y;
    GdkScreen *screen;
    GtkOrientation orientation;
    
    if (gtk_widget_get_visible(app->status_window)) {
        gtk_widget_hide(app->status_window);
        return;
    }
    
    /*screen = gtk_status_icon_get_screen(status_icon);
    gtk_status_icon_get_geometry(status_icon, &screen, &orientation);
    
    gdk_screen_get_monitor_geometry(screen, 0, NULL);*/
    
    gtk_window_set_position(GTK_WINDOW(app->status_window), GTK_WIN_POS_MOUSE);
    gtk_widget_show_all(app->status_window);
}

gboolean status_window_focus_out(GtkWidget *widget, GdkEventFocus *event, gpointer data) {
    PBSndVol *app = (PBSndVol *)data;
    gtk_widget_hide(app->status_window);
    return FALSE;
}

void show_mixer_window(GtkMenuItem *item, gpointer data) {
    PBSndVol *app = (PBSndVol *)data;
    gtk_widget_show(app->window);
}

void ext_vol_toggled(GtkCheckMenuItem *item, gpointer data) {
    PBSndVol *app;
    GList *l;
    PBSndVolSiloData *sd;
    gdouble top;
    gboolean want_on;

    app = (PBSndVol *)data;
    want_on = gtk_check_menu_item_get_active(item);

    if (want_on) {
        GtkWidget *dlg;
        GtkWidget *dlgicon;
        gint resp;
        dlg = gtk_message_dialog_new(GTK_WINDOW(app->window), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, "Raising volume above 100%% can cause audio distortion, hearing damage, and in some cases speaker or hardware damage. Are you sure you want to enable it?");
        gtk_dialog_add_button(GTK_DIALOG(dlg), GTK_STOCK_NO, GTK_RESPONSE_NO);
        gtk_dialog_add_button(GTK_DIALOG(dlg), GTK_STOCK_YES, GTK_RESPONSE_YES);
        gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_NO);
        dlgicon = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
        gtk_message_dialog_set_image(GTK_MESSAGE_DIALOG(dlg), dlgicon);
        gtk_widget_show(dlgicon);
        resp = gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
        if (resp != GTK_RESPONSE_YES) {
            g_signal_handlers_block_by_func(item, G_CALLBACK(ext_vol_toggled), data);
            gtk_check_menu_item_set_active(item, FALSE);
            g_signal_handlers_unblock_by_func(item, G_CALLBACK(ext_vol_toggled), data);
            return;
        }
    }

    app->ext_vol = want_on;
    top = app->ext_vol ? 200 : 100;

    for (l = app->silos; l; l = l->next) {
        sd = (PBSndVolSiloData *)l->data;
        gtk_range_set_range(GTK_RANGE(sd->vol_scale), 0, top);
        if (!app->ext_vol && gtk_range_get_value(GTK_RANGE(sd->vol_scale)) > 100) {
            gtk_range_set_value(GTK_RANGE(sd->vol_scale), 100);
        }
    }
}

void silo_allocate_cb(GtkWidget *widget, GdkRectangle *allocation, gpointer udata) {
    GtkWidget *l;
    
    l = gtk_frame_get_label_widget(GTK_FRAME(widget));
    if (l) {
        GtkAllocation a;
        
        gtk_widget_get_allocation(l, &a);
        gtk_widget_set_size_request(widget, MAX(a.width + 32, allocation->width), -1);
    }
}


GtkWidget *make_icon_image(const gchar *ico_name, const gchar *fallback, GtkIconSize size) {
    GtkIconTheme *theme;
    GtkWidget *img;
    gint width, height;

    theme = gtk_icon_theme_get_default();
    gtk_icon_size_lookup(size, &width, &height);
    
    if (ico_name && gtk_icon_theme_has_icon(theme, ico_name)) {
        return gtk_image_new_from_icon_name(ico_name, size);
    }
    
    if (fallback && gtk_icon_theme_has_icon(theme, fallback)) {
        return gtk_image_new_from_icon_name(fallback, size);
    }
    
    img = gtk_image_new_from_stock(GTK_STOCK_MISSING_IMAGE, size);
    return img;
}


PBSndVolSiloData *create_silo(PBSndVol *app, gint type, guint32 index, const gchar *text, const gchar *text2, const gchar *ico_name, gint show_name) {
    PBSndVolSiloData *sd;
    GtkWidget *vbox;
    GtkWidget *balbox;
    GtkWidget *icon;
    GtkWidget *a;
    GtkWidget *l;
    GtkWidget *img;
    GtkWidget *flabel;
    GtkWidget *namelabel;
    const gchar *fallback_ico;
    GtkAllocation b;
    
    sd = g_malloc0(sizeof(PBSndVolSiloData));
    sd->app = app;
    sd->type = type;
    sd->index = index;
    
    sd->frame = gtk_frame_new(show_name ? text : NULL);
    l = gtk_frame_get_label_widget(GTK_FRAME(sd->frame));
    if (l) {
        gtk_label_set_ellipsize(GTK_LABEL(l), PANGO_ELLIPSIZE_END);
        gtk_label_set_max_width_chars(GTK_LABEL(l), SILO_LABEL_MAX_CHARS);
    }    
    gtk_container_set_border_width(GTK_CONTAINER(sd->frame), 5);
    gtk_frame_set_label_align(GTK_FRAME(sd->frame), 0.5, 0.5);
    gtk_frame_set_shadow_type(GTK_FRAME(sd->frame), GTK_SHADOW_ETCHED_IN);
    g_signal_connect(sd->frame, "size-allocate", G_CALLBACK(silo_allocate_cb), NULL);
    
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(sd->frame), vbox);

    if (type == 0) {
        fallback_ico = "audio-card";
    } else if (type == 2) {
        fallback_ico = "audio-input-microphone";
    } else {
        fallback_ico = NULL;
    }
    img = make_icon_image(ico_name, fallback_ico, GTK_ICON_SIZE_DND);
    gtk_box_pack_start(GTK_BOX(vbox), img, FALSE, FALSE, 0);

    if (!show_name) {
        namelabel = gtk_label_new(text);
        gtk_label_set_ellipsize(GTK_LABEL(namelabel), PANGO_ELLIPSIZE_END);
        gtk_label_set_max_width_chars(GTK_LABEL(namelabel), SILO_LABEL_MAX_CHARS);
        gtk_box_pack_start(GTK_BOX(vbox), namelabel, FALSE, FALSE, 0);
    } else {
        icon = gtk_label_new(text2);
        gtk_box_pack_start(GTK_BOX(vbox), icon, FALSE, FALSE, 0);
    }

    balbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), balbox, FALSE, FALSE, 10);
    
    icon = gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start(GTK_BOX(balbox), icon, FALSE, FALSE, 0);

    sd->bal_scale = gtk_hscale_new_with_range(-1, 1, 0.01);
    gtk_widget_set_size_request(sd->bal_scale, 50, -1);
    gtk_scale_set_draw_value(GTK_SCALE(sd->bal_scale), FALSE);
    gtk_scale_add_mark(GTK_SCALE(sd->bal_scale), 0, GTK_POS_TOP, NULL);
    gtk_box_pack_start(GTK_BOX(balbox), sd->bal_scale, TRUE, TRUE, 0);
    g_signal_connect(sd->bal_scale, "value-changed", G_CALLBACK(bal_changed), sd);
    g_signal_connect(sd->bal_scale, "button-press-event", G_CALLBACK(bal_scale_press_cb), sd);

    icon = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_end(GTK_BOX(balbox), icon, FALSE, FALSE, 0);

    sd->vol_scale = gtk_vscale_new_with_range(0, app->ext_vol ? 200 : 100, 1);
    gtk_range_set_inverted(GTK_RANGE(sd->vol_scale), TRUE);
    gtk_widget_set_size_request(sd->vol_scale, -1, 100);
    gtk_scale_set_draw_value(GTK_SCALE(sd->vol_scale), FALSE);
    gtk_scale_add_mark(GTK_SCALE(sd->vol_scale), 50, GTK_POS_LEFT, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), sd->vol_scale, TRUE, TRUE, 0);
    g_signal_connect(sd->vol_scale, "value-changed", G_CALLBACK(vol_changed), sd);
    g_signal_connect(sd->vol_scale, "button-press-event", G_CALLBACK(vol_scale_press_cb), sd);

    a = gtk_alignment_new(0.5, 0.5, 0, 0);
    sd->mute_check = gtk_check_button_new_with_label("Mute");
    gtk_container_add(GTK_CONTAINER(a), sd->mute_check);
    gtk_box_pack_end(GTK_BOX(vbox), a, FALSE, FALSE, 0);
    g_signal_connect(sd->mute_check, "toggled", G_CALLBACK(mute_toggled), sd);

    app->silos = g_list_append(app->silos, sd);
    
	gtk_widget_set_size_request(sd->frame, 155, -1);

    return sd;
}

void sink_cb(pa_context *c, const pa_sink_info *i, gint eol, void *userdata) {
    PBSndVol *app;
    PBSndVolSiloData *sd;
    const gchar *ico;
    const gchar *name;
    GtkWidget *mi;
    GtkWidget *sub;
    GtkWidget *pmi;
    PBSndVolPortData *pd;
    gint j;

    app = (PBSndVol *)userdata;
    if (eol > 0) {
        app->menu_built = 1;
        if (app->sink_count == 0 && !app->no_devices_warned) {
            GtkWidget *dlg;
            app->no_devices_warned = 1;
            dlg = gtk_message_dialog_new(GTK_WINDOW(app->window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "No audio outputs or devices were found.");
            gtk_dialog_run(GTK_DIALOG(dlg));
            gtk_widget_destroy(dlg);
        }
        return;
    }
    app->sink_count++;
    
    sd = find_silo(app, 0, i->index);
    ico = pa_proplist_gets(i->proplist, PA_PROP_DEVICE_ICON_NAME);
    if (!ico) {
        ico = pa_proplist_gets(i->proplist, PA_PROP_DEVICE_FORM_FACTOR);
    }
    if (!ico) {
        ico = "audio-card";
    }
    
    const gchar *dev_name = pa_proplist_gets(i->proplist, PA_PROP_DEVICE_PRODUCT_NAME);
    if (!dev_name) {
        dev_name = pa_proplist_gets(i->proplist, "alsa.card_name");
        if (!dev_name) {
            dev_name = i->description;
        }
    }
    
    const gchar *port_name = i->active_port ? i->active_port->description : dev_name;
    
    if (!sd) {
        sd = create_silo(app, 0, i->index, dev_name, port_name, ico, 1);
        gtk_box_pack_start(GTK_BOX(app->master_box), sd->frame, FALSE, TRUE, 0);
        gtk_widget_show_all(app->master_box);
    }
    update_silo(sd, &i->volume, i->mute);

    if (!app->menu_built) {
        mi = gtk_menu_item_new_with_label(dev_name);
        gtk_menu_shell_append(GTK_MENU_SHELL(app->devices_menu), mi);
        
        if (i->n_ports > 0) {
            sub = gtk_menu_new();
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), sub);
            for (j = 0; j < i->n_ports; ++j) {
                pd = g_malloc0(sizeof(PBSndVolPortData));
                pd->app = app;
                pd->index = i->index;
                pd->is_sink = 1;
                pd->port_name = g_strdup(i->ports[j]->name);
                app->port_items = g_list_append(app->port_items, pd);
                
                pmi = gtk_check_menu_item_new_with_label(i->ports[j]->description);
                if (i->active_port && !strcmp(i->active_port->name, i->ports[j]->name)) {
                    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(pmi), TRUE);
                }
                g_signal_connect(pmi, "toggled", G_CALLBACK(port_toggled_cb), pd);
                gtk_menu_shell_append(GTK_MENU_SHELL(sub), pmi);
            }
        }
        gtk_widget_show_all(app->devices_menu);
    }
}

void source_cb(pa_context *c, const pa_source_info *i, gint eol, void *userdata) {
    PBSndVol *app;
    PBSndVolSiloData *sd;
    const gchar *ico;
    const gchar *name;

    if (eol > 0) {
        return;
    }
    app = (PBSndVol *)userdata;
    
    if (strstr(i->name, "monitor")) {
        return;
    }

    sd = find_silo(app, 2, i->index);
    ico = pa_proplist_gets(i->proplist, PA_PROP_DEVICE_ICON_NAME);
    if (!ico) {
        ico = pa_proplist_gets(i->proplist, PA_PROP_DEVICE_FORM_FACTOR);
    }
    if (!ico) {
        ico = "audio-input-microphone";
    }
    
    if (i->active_port) {
        name = i->active_port->description;
    } else {
        name = pa_proplist_gets(i->proplist, PA_PROP_DEVICE_DESCRIPTION);
        if (!name) {
            name = pa_proplist_gets(i->proplist, PA_PROP_DEVICE_PRODUCT_NAME);
            if (!name) {
                name = pa_proplist_gets(i->proplist, "alsa.card_name");
                if (!name) {
                    name = i->description;
                }
            }
        }
    }
    
    if (!sd) {
        sd = create_silo(app, 2, i->index, name, name, ico, 0);
        gtk_box_pack_start(GTK_BOX(app->hw_box), sd->frame, FALSE, FALSE, 0);
        gtk_widget_show_all(app->hw_box);
        gtk_widget_hide(app->hw_empty);
    }
    update_silo(sd, &i->volume, i->mute);
}

void sink_input_cb(pa_context *c, const pa_sink_input_info *i, gint eol, void *userdata) {
    PBSndVol *app;
    PBSndVolSiloData *sd;
    const gchar *ico;
    const gchar *name;

    if (eol > 0) {
        return;
    }
    app = (PBSndVol *)userdata;
    
    sd = find_silo(app, 1, i->index);
    ico = pa_proplist_gets(i->proplist, PA_PROP_APPLICATION_ICON_NAME);
    if (!ico) {
        ico = pa_proplist_gets(i->proplist, PA_PROP_MEDIA_ICON_NAME);
    }
    if (!ico) {
        ico = "audio-x-generic";
    }
    
    name = pa_proplist_gets(i->proplist, PA_PROP_APPLICATION_NAME);
    if (!name) {
        name = pa_proplist_gets(i->proplist, PA_PROP_MEDIA_NAME);
    }
    if (!name) {
        name = pa_proplist_gets(i->proplist, PA_PROP_APPLICATION_ID);
    }
    if (!name) {
        name = "Application";
    }
    
    if (!sd) {
        sd = create_silo(app, 1, i->index, name, name, ico, 0);
        gtk_box_pack_start(GTK_BOX(app->app_box), sd->frame, FALSE, FALSE, 0);
        gtk_widget_show_all(app->app_box);
        gtk_widget_hide(app->app_empty);
    }
    update_silo(sd, &i->volume, i->mute);
}

void subscribe_cb(pa_context *c, pa_subscription_event_type_t t, guint32 idx, void *userdata) {
    PBSndVol *app;
    gint facility;
    gint type;
    
    app = (PBSndVol *)userdata;
    facility = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
    type = t & PA_SUBSCRIPTION_EVENT_TYPE_MASK;
    
    if (type == PA_SUBSCRIPTION_EVENT_REMOVE) {
        GList *l;
        PBSndVolSiloData *sd;
        gint stype;
        
        stype = -1;
        if (facility == PA_SUBSCRIPTION_EVENT_SINK) {
            stype = 0;
        } else if (facility == PA_SUBSCRIPTION_EVENT_SINK_INPUT) {
            stype = 1;
        } else if (facility == PA_SUBSCRIPTION_EVENT_SOURCE) {
            stype = 2;
        }
        
        if (stype != -1) {
            for (l = app->silos; l; l = l->next) {
                sd = (PBSndVolSiloData *)l->data;
                if (sd->type == stype && sd->index == idx) {
                    gtk_widget_destroy(sd->frame);
                    app->silos = g_list_remove(app->silos, sd);
                    g_free(sd);
                    break;
                }
            }
            if (stype == 1) {
                update_empty_state(app->app_box, app->app_empty);
            } else if (stype == 2) {
                update_empty_state(app->hw_box, app->hw_empty);
            } else if (stype == 0) {
                app->sink_count--;
            }
        }
    } else if (type == PA_SUBSCRIPTION_EVENT_NEW || type == PA_SUBSCRIPTION_EVENT_CHANGE) {
        if (facility == PA_SUBSCRIPTION_EVENT_SINK) {
            if (type == PA_SUBSCRIPTION_EVENT_NEW) {
                free_port_items(app);
                clear_menu(app->devices_menu);
                app->menu_built = 0;
                pa_context_get_sink_info_list(c, sink_cb, app);
            } else {
                pa_context_get_sink_info_by_index(c, idx, sink_cb, app);
            }
        } else if (facility == PA_SUBSCRIPTION_EVENT_SINK_INPUT) {
            pa_context_get_sink_input_info(c, idx, sink_input_cb, app);
        } else if (facility == PA_SUBSCRIPTION_EVENT_SOURCE) {
            pa_context_get_source_info_by_index(c, idx, source_cb, app);
        }
    }
}

void state_cb(pa_context *c, void *udata) {
    PBSndVol *app;
    
    app = (PBSndVol *)udata;
    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_READY:
            pa_context_get_sink_info_list(c, sink_cb, app);
            pa_context_get_source_info_list(c, source_cb, app);
            pa_context_get_sink_input_info_list(c, sink_input_cb, app);
            pa_context_set_subscribe_callback(c, subscribe_cb, app);
            pa_context_subscribe(c, PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SOURCE | PA_SUBSCRIPTION_MASK_SINK_INPUT, NULL, NULL);
            break;
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
            gtk_main_quit();
            break;            
        default:
            break;
    }
}

gint main(gint argc, gchar *argv[]) {
    PBSndVol app;
    GtkWidget *vbox;
    GtkWidget *menubar;
    GtkWidget *devicemi;
    GtkWidget *helpmi;
    GtkWidget *hbox;
    GtkWidget *notebook;
    GtkWidget *scroll;
    GtkWidget *label;
    
    memset(&app, 0, sizeof(PBSndVol));
    gtk_init(&argc, &argv);
    
    app.paloop = pa_glib_mainloop_new(NULL);
    app.paapi = pa_glib_mainloop_get_api(app.paloop);
    app.pactx = pa_context_new(app.paapi, "PBSndVol");
    pa_context_set_state_callback(app.pactx, state_cb, &app);
    pa_context_connect(app.pactx, NULL, PA_CONTEXT_NOFLAGS, NULL);
    
    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "Sound Mixer");
    gtk_window_set_default_size(GTK_WINDOW(app.window), 640, 480);
    gtk_window_set_position(GTK_WINDOW(app.window), GTK_WIN_POS_CENTER);
    g_signal_connect(G_OBJECT(app.window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(app.window), vbox);

    menubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    devicemi = gtk_menu_item_new_with_label("Devices");
    app.devices_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(devicemi), app.devices_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), devicemi);

    app.ext_vol_item = gtk_check_menu_item_new_with_label("Allow volume up to 200%");
    gtk_menu_shell_append(GTK_MENU_SHELL(app.devices_menu), app.ext_vol_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(app.devices_menu), gtk_separator_menu_item_new());
    g_signal_connect(app.ext_vol_item, "toggled", G_CALLBACK(ext_vol_toggled), &app);

    helpmi = gtk_menu_item_new_with_label("Help");
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), helpmi);
    
    GtkWidget *help_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(helpmi), help_menu);
    
    GtkWidget *about_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
    g_signal_connect(about_item, "activate", G_CALLBACK(about_cb), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), about_item);

    app.status_icon = gtk_status_icon_new_from_icon_name("audio-volume-medium");
    gtk_status_icon_set_tooltip_text(app.status_icon, "Sound Mixer");
    g_signal_connect(app.status_icon, "popup-menu", G_CALLBACK(status_icon_popup), &app);
    g_signal_connect(app.status_icon, "activate", G_CALLBACK(status_icon_clicked), &app);
    
    app.status_menu = gtk_menu_new();
    
    GtkWidget *mixer_item = gtk_menu_item_new_with_label("Open Mixer");
    g_signal_connect(mixer_item, "activate", G_CALLBACK(show_mixer_window), &app);
    gtk_menu_shell_append(GTK_MENU_SHELL(app.status_menu), mixer_item);
    
    GtkWidget *quit_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
    g_signal_connect(quit_item, "activate", G_CALLBACK(gtk_main_quit), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(app.status_menu), quit_item);
    
    app.status_window = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_decorated(GTK_WINDOW(app.status_window), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(app.status_window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(app.status_window), TRUE);
    g_signal_connect(app.status_window, "focus-out-event", G_CALLBACK(status_window_focus_out), &app);
    
    GtkWidget *vol_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vol_vbox), 10);
    gtk_container_add(GTK_CONTAINER(app.status_window), vol_vbox);
    
    GtkWidget *vol_label = gtk_label_new("Volume");
    gtk_box_pack_start(GTK_BOX(vol_vbox), vol_label, FALSE, FALSE, 0);
    
    app.status_vol_scale = gtk_vscale_new_with_range(0, 100, 1);
    gtk_range_set_inverted(GTK_RANGE(app.status_vol_scale), TRUE);
    gtk_widget_set_size_request(app.status_vol_scale, -1, 100);
    gtk_scale_set_draw_value(GTK_SCALE(app.status_vol_scale), FALSE);
    gtk_box_pack_start(GTK_BOX(vol_vbox), app.status_vol_scale, TRUE, TRUE, 0);
    g_signal_connect(app.status_vol_scale, "value-changed", G_CALLBACK(status_vol_changed), &app);
    
    app.status_mute_check = gtk_check_button_new_with_label("Mute");
    gtk_box_pack_start(GTK_BOX(vol_vbox), app.status_mute_check, FALSE, FALSE, 0);
    g_signal_connect(app.status_mute_check, "toggled", G_CALLBACK(status_mute_toggled), &app);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    app.master_box = gtk_vbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), app.master_box, FALSE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), gtk_vseparator_new(), FALSE, FALSE, 0);

    notebook = gtk_notebook_new();
    gtk_box_pack_end(GTK_BOX(hbox), notebook, TRUE, TRUE, 0);

    label = gtk_label_new("Application Mixer");
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scroll, label);
    
    app.app_box = gtk_hbox_new(FALSE, 0);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), app.app_box);
    app.app_empty = gtk_label_new("No programs are playing audio at this time.");
    gtk_box_pack_start(GTK_BOX(app.app_box), app.app_empty, TRUE, TRUE, 0);
    
    label = gtk_label_new("Hardware Mixer");
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_ETCHED_IN);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scroll, label);
    
    app.hw_box = gtk_hbox_new(FALSE, 0);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), app.hw_box);
    app.hw_empty = gtk_label_new("No hardware inputs are avalible.");
    gtk_box_pack_start(GTK_BOX(app.hw_box), app.hw_empty, TRUE, TRUE, 0);
    
    gtk_widget_show_all(app.window);
    gtk_main();

    pa_context_unref(app.pactx);
    pa_glib_mainloop_free(app.paloop);
    
    return 0;
}
