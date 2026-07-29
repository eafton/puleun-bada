#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

enum {
    COLUMN_ICO,
    COLUMN_NAME,
    COLUMN_SIZE,
    COLUMN_TYPE,
    COLUMN_MODIFIED,
    N_COLUMNS
};

enum {
    COL_PIXBUF,
    COL_TEXT,
    NUM_COLUMNS
};

typedef struct {
	GFile *root;
	
	GtkTreeStore *combo_store;
} LRApp;

typedef struct {
    GtkTreeStore *treestore;
    GtkTreeIter *p_iter;
    GFile *dir;
    GHashTable *icon_cache;
} AsyncFolderTask;

void lr_create_menus(GtkWidget *menubar) {
	GtkWidget *file_mi;
	GtkWidget *commands_mi;
	GtkWidget *tools_mi;
	GtkWidget *favorites_mi;
	GtkWidget *options_mi;
	GtkWidget *help_mi;

	file_mi = gtk_menu_item_new_with_mnemonic("_File");
	commands_mi = gtk_menu_item_new_with_mnemonic("_Commands");
	tools_mi = gtk_menu_item_new_with_mnemonic("Tool_s");
	favorites_mi = gtk_menu_item_new_with_mnemonic("Fav_orites");
	options_mi = gtk_menu_item_new_with_mnemonic("Optio_ns");
	help_mi = gtk_menu_item_new_with_mnemonic("_Help");

	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file_mi);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), commands_mi);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), tools_mi);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), favorites_mi);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), options_mi);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), help_mi);
}

void on_combo_changed(GtkComboBox *combo_box, gpointer user_data) {
    GtkTreeIter iter;
    
    if (gtk_combo_box_get_active_iter(combo_box, &iter)) {
		GtkWidget *entry;
        GtkTreeModel *model;
        GdkPixbuf *pixbuf;
         
		pixbuf = NULL;
		model = gtk_combo_box_get_model(combo_box);
        gtk_tree_model_get(model, &iter, COL_PIXBUF, &pixbuf, -1);
        
        entry = gtk_bin_get_child(GTK_BIN(combo_box));        
        if (entry && GTK_IS_ENTRY(entry)) {
            gtk_entry_set_icon_from_pixbuf(GTK_ENTRY(entry), GTK_ENTRY_ICON_PRIMARY, pixbuf);
        }
        if (pixbuf) {
            g_object_unref(pixbuf);
        }
    }
}

GtkWidget *create_combo(GtkWidget *widget) {
	GtkWidget *combo;
	GtkCellRenderer *renderer_text;
	GtkCellRenderer *renderer_pixbuf;
    GtkListStore *store;
    GdkPixbuf *icon;
	GtkTreeIter iter;

    store = gtk_list_store_new(NUM_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING);
    
    icon = gtk_widget_render_icon(widget, GTK_STOCK_DIRECTORY, GTK_ICON_SIZE_MENU, NULL);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, COL_PIXBUF, icon, COL_TEXT, "Test", -1);
	g_object_unref(icon);

    combo = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(store));
	g_signal_connect(combo, "changed", G_CALLBACK(on_combo_changed), NULL);
	gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(combo), COL_TEXT);
    gtk_cell_layout_clear(GTK_CELL_LAYOUT(combo));
	g_object_unref(store); 

    renderer_pixbuf = gtk_cell_renderer_pixbuf_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer_pixbuf, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer_pixbuf, "pixbuf", COL_PIXBUF, NULL);

    renderer_text = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer_text, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer_text, "text", COL_TEXT, NULL);

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
	return combo;
}

void lr_create_secondary_tool_items(GtkToolbar *toolbar) {
	GtkToolItem *tool_item;
	GtkWidget *widget;
	
	tool_item = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_SMALL_TOOLBAR), NULL);
	gtk_toolbar_insert(toolbar, tool_item, -1);

	tool_item = gtk_separator_tool_item_new();
	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(tool_item), TRUE);
	gtk_toolbar_insert(toolbar, tool_item, -1);

	widget = create_combo(GTK_WIDGET(toolbar));
	tool_item = gtk_tool_item_new();
	gtk_tool_item_set_expand(tool_item, TRUE);
	gtk_container_add(GTK_CONTAINER(tool_item), widget);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);


}

GtkWidget *extract_to_icon(GtkToolbar *tb) {
	GtkWidget *ret;
	GtkIconInfo *info;
	GdkPixbuf *archive;
	gint w, h;
	
	gtk_icon_size_lookup(gtk_toolbar_get_icon_size(tb), &w, &h);
	info = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_for_screen(gtk_widget_get_screen(GTK_WIDGET(tb))), "archive-extract", (w+h)/2, 0);
	if (info) {
		ret = gtk_image_new_from_icon_name("archive-extract", gtk_toolbar_get_icon_size(tb));
		gtk_icon_info_free(info);
	} else {
		GdkPixbuf *base;
		GdkPixbuf *arrow;

		base = gtk_icon_theme_load_icon(gtk_icon_theme_get_for_screen(gtk_widget_get_screen(GTK_WIDGET(tb))), "package-x-generic", (w+h)/2, 0, NULL);
		if (!base) {
			base = gtk_widget_render_icon(GTK_WIDGET(tb), GTK_STOCK_DIRECTORY, gtk_toolbar_get_icon_size(tb), NULL);
		}
		arrow = gtk_widget_render_icon(GTK_WIDGET(tb), GTK_STOCK_REDO, gtk_icon_size_register("composite-size", 16, 16), NULL);
		gdk_pixbuf_composite(arrow, base, w - 16, 0, 16, 16, w - 16, 0, 1, 1, GDK_INTERP_TILES, 255);
		ret = gtk_image_new_from_pixbuf(base);
		gdk_pixbuf_unref(base);
		gdk_pixbuf_unref(arrow);
	}
	
	return ret;
}

void lr_create_tool_items(GtkToolbar *toolbar) {
	GtkToolItem *tool_item;
	
	tool_item = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_LARGE_TOOLBAR), "Add");
	gtk_toolbar_insert(toolbar, tool_item, -1);

	tool_item = gtk_tool_button_new(extract_to_icon(toolbar), "Extract To");
	gtk_toolbar_insert(toolbar, tool_item, -1);

	tool_item = gtk_tool_button_new(NULL, "Test");
	gtk_toolbar_insert(toolbar, tool_item, -1);

	tool_item = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_INDEX, GTK_ICON_SIZE_LARGE_TOOLBAR), "View");
	gtk_toolbar_insert(toolbar, tool_item, -1);

	tool_item = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_DELETE, GTK_ICON_SIZE_LARGE_TOOLBAR), "Delete");
	gtk_toolbar_insert(toolbar, tool_item, -1);
	
	tool_item = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_LARGE_TOOLBAR), "Find");
	gtk_toolbar_insert(toolbar, tool_item, -1);

	tool_item = gtk_tool_button_new(NULL, "Wizard");
	gtk_toolbar_insert(toolbar, tool_item, -1);
	
	tool_item = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_INFO, GTK_ICON_SIZE_LARGE_TOOLBAR), "Info");
	gtk_toolbar_insert(toolbar, tool_item, -1);
	
	tool_item = gtk_separator_tool_item_new();
	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(tool_item), TRUE);
	gtk_toolbar_insert(toolbar, tool_item, -1);

	tool_item = gtk_tool_button_new(NULL, "Repair");
	gtk_toolbar_insert(toolbar, tool_item, -1);
}

GtkWidget *create_view() {
	GtkListStore *list_store;
	GtkWidget *tree_view;
	GtkCellRenderer *renderer;
    GtkCellRenderer *renderer_pixbuf;
    GtkTreeViewColumn *col_name;
    GdkPixbuf *pxbuf;
	GtkTreeIter iter;

	list_store = gtk_list_store_new(N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
	g_object_unref(list_store);
	
    col_name = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col_name, "Name");

    renderer_pixbuf = gtk_cell_renderer_pixbuf_new();
 	renderer = gtk_cell_renderer_text_new();
	
	gtk_tree_view_column_pack_start(col_name, renderer_pixbuf, FALSE);
    gtk_tree_view_column_add_attribute(col_name, renderer_pixbuf, "pixbuf", COLUMN_ICO);
    gtk_tree_view_column_pack_start(col_name, renderer, TRUE);
    gtk_tree_view_column_add_attribute(col_name, renderer, "text", COLUMN_NAME);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(tree_view), col_name, 0);

	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view), -1, "Size", renderer, "text", COLUMN_SIZE, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view), -1, "Type", renderer, "text", COLUMN_TYPE, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view), -1, "Modified", renderer, "text", COLUMN_MODIFIED, NULL);

	gtk_list_store_append(list_store, &iter);
    pxbuf = gtk_widget_render_icon(tree_view, GTK_STOCK_FILE, GTK_ICON_SIZE_MENU, NULL);
	gtk_list_store_set(list_store, &iter, COLUMN_ICO, pxbuf, COLUMN_NAME, "document.txt", COLUMN_SIZE, "14 KB", COLUMN_TYPE, "Text Document", COLUMN_MODIFIED, "2026-07-08 14:30", -1); 
	gdk_pixbuf_unref(pxbuf);
	
	return tree_view;
}


#if GTK_CHECK_VERSION(3, 0, 0)
gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	GtkStyleContext *context;
    GtkAllocation allocation;
   
    gtk_widget_get_allocation(widget, &allocation);
    context = gtk_widget_get_style_context(widget);
    gtk_render_background(context, cr, 0, 0, allocation.width, allocation.height);
    gtk_style_context_save(context);
    gtk_style_context_add_class(context, GTK_STYLE_CLASS_GRIP);
    gtk_render_handle(context, cr, 0, 0, allocation.width, allocation.height);
    gtk_style_context_restore(context);
    return TRUE;
}
#else
gboolean on_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data) {
    GtkAllocation allocation;
    
    gtk_widget_get_allocation(widget, &allocation);
    gdk_draw_rectangle(widget->window, widget->style->bg_gc[GTK_STATE_NORMAL], TRUE, 0, 0, allocation.width, allocation.height);
    gtk_paint_resize_grip(widget->style, widget->window, GTK_STATE_NORMAL, &event->area, widget, "statusbar", GDK_WINDOW_EDGE_SOUTH_EAST, 0, 0, allocation.width, allocation.height);
    return TRUE;
}
#endif

gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    GtkWidget *window;
    
    window = gtk_widget_get_toplevel(widget);
    if (event->button == 1 && GTK_IS_WINDOW(window)) {
        gtk_window_begin_resize_drag(GTK_WINDOW(window), GDK_WINDOW_EDGE_SOUTH_EAST, event->button, (gint)event->x_root, (gint)event->y_root, event->time);
        return TRUE;
    }
    return FALSE;
}

void set_cursor(GtkWidget *handle) {
	GdkCursor *cursor;
#if GTK_CHECK_VERSION(3, 0, 0)
    GdkDisplay *display;
    
    display = gtk_widget_get_display(handle);
	cursor = gdk_cursor_new_for_display(display, GDK_SIZING);
    gdk_window_set_cursor(gtk_widget_get_window(handle), cursor);
    g_object_unref(cursor);
#else
	cursor = gdk_cursor_new(GDK_SIZING);
    gdk_window_set_cursor(handle->window, cursor);
    gdk_cursor_unref(cursor);
#endif
}

GtkWidget *create_statusbar() {
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *disk_button;
	GtkWidget *pw_button;
	GtkWidget *label;
	GtkWidget *fake;
	
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	disk_button = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(disk_button), gtk_image_new_from_stock(GTK_STOCK_HARDDISK, GTK_ICON_SIZE_BUTTON));
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_button_set_always_show_image(GTK_BUTTON(disk_button), TRUE);
#else
	gtk_widget_show(gtk_button_get_image(GTK_BUTTON(disk_button)));
#endif
	gtk_button_set_relief(GTK_BUTTON(disk_button), GTK_RELIEF_NONE);
	gtk_box_pack_start(GTK_BOX(hbox), disk_button, FALSE, FALSE, 0);

	pw_button = gtk_button_new();
	gtk_button_set_image(GTK_BUTTON(pw_button), gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_BUTTON));
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_button_set_always_show_image(GTK_BUTTON(pw_button), TRUE);
#else
	gtk_widget_show(gtk_button_get_image(GTK_BUTTON(pw_button)));
#endif
	gtk_button_set_relief(GTK_BUTTON(pw_button), GTK_RELIEF_NONE);
	gtk_box_pack_start(GTK_BOX(hbox), pw_button, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(hbox), gtk_vseparator_new(), FALSE, FALSE, 0);
	label = gtk_label_new("Selected folder");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 8);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_vseparator_new(), FALSE, FALSE, 0);
	label = gtk_label_new("Total SOmething something");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

	fake = gtk_drawing_area_new();
    gtk_widget_set_size_request(fake, 24, 24); 
    gtk_widget_add_events(fake, GDK_BUTTON_PRESS_MASK);
	gtk_widget_realize(fake);
#if GTK_CHECK_VERSION(3, 0, 0)
	g_signal_connect(fake, "draw", G_CALLBACK(on_draw), NULL);
#else
	g_signal_connect(fake, "expose-event", G_CALLBACK(on_expose), NULL);
#endif
	g_signal_connect(fake, "button-press-event", G_CALLBACK(on_button_press), NULL);

    gtk_box_pack_end(GTK_BOX(hbox), fake, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), label, TRUE, TRUE, 8);

	return frame;
}

GdkPixbuf *get_folder_icon(GHashTable *cache, GFileInfo *info) {
    GIcon *gicon;
    GdkPixbuf *pixbuf;
    gchar *icon_name;

    gicon = g_file_info_get_icon(info);
    if (!gicon || !G_IS_THEMED_ICON(gicon)) {
        return NULL;
    }

    icon_name = g_strjoinv(",", (gchar **)g_themed_icon_get_names(G_THEMED_ICON(gicon)));
    pixbuf = (GdkPixbuf *)g_hash_table_lookup(cache, icon_name);

    if (!pixbuf) {
        GtkIconTheme *theme;
        GtkIconInfo *icon_info;

        theme = gtk_icon_theme_get_default();
        icon_info = gtk_icon_theme_lookup_by_gicon(theme, gicon, 16, GTK_ICON_LOOKUP_USE_BUILTIN);

        if (icon_info) {
            if (!gtk_icon_info_get_filename(icon_info)) {
                pixbuf = gtk_icon_info_get_builtin_pixbuf(icon_info);
                if (pixbuf) {
                    g_object_ref(pixbuf);
                }
            } else {
                pixbuf = gtk_icon_info_load_icon(icon_info, NULL);
            }
            g_object_unref(icon_info);
        }

        if (pixbuf) {
            g_hash_table_insert(cache, g_strdup(icon_name), pixbuf);
        }
    }

    g_free(icon_name);
    return pixbuf;
}

void start_async_folder_read(GtkTreeStore *treestore, GFile *file, GtkTreeIter *p_iter, GHashTable *cache);

void on_children_enumerated(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GFile *file;
    AsyncFolderTask *task;
    GFileEnumerator *enumerator;
    GFileInfo *info;
    GError *error;

    file = G_FILE(source_object);
    task = (AsyncFolderTask *)user_data;
    error = NULL;

    enumerator = g_file_enumerate_children_finish(file, res, &error);
    if (error != NULL) {
        g_error_free(error);
        if (task->p_iter) {
            g_free(task->p_iter);
        }
        g_object_unref(task->dir);
        g_free(task);
        return;
    }

    while ((info = g_file_enumerator_next_file(enumerator, NULL, NULL))) {
        if (g_file_info_get_file_type(info) == G_FILE_TYPE_DIRECTORY) {
            GtkTreeIter iter;
            GFile *chld;
            GtkTreeIter *next_p_iter;
            GdkPixbuf *folder_pixbuf;

            gtk_tree_store_append(task->treestore, &iter, task->p_iter);
            folder_pixbuf = get_folder_icon(task->icon_cache, info);

            gtk_tree_store_set(task->treestore, &iter, COL_PIXBUF, folder_pixbuf, COL_TEXT, g_file_info_get_name(info), -1);

			puts(g_file_info_get_name(info));
			gdk_pixbuf_save(folder_pixbuf, g_strconcat(g_file_info_get_name(info), ".png", NULL), "png", NULL, NULL);

            chld = g_file_get_child(file, g_file_info_get_name(info));
            next_p_iter = g_new(GtkTreeIter, 1);
            *next_p_iter = iter;

            start_async_folder_read(task->treestore, chld, next_p_iter, task->icon_cache);
            g_object_unref(chld);
        }
        g_object_unref(info);
    }

    g_object_unref(enumerator);
    if (task->p_iter) {
        g_free(task->p_iter);
    }
    g_object_unref(task->dir);
    g_free(task);
}

void start_async_folder_read(GtkTreeStore *treestore, GFile *file, GtkTreeIter *p_iter, GHashTable *cache) {
    AsyncFolderTask *task;

    task = g_new0(AsyncFolderTask, 1);
    task->treestore = treestore;
    task->p_iter = p_iter;
    task->dir = g_object_ref(file);
    task->icon_cache = cache;

    g_file_enumerate_children_async(file, "standard::name,standard::type,standard::icon", G_FILE_QUERY_INFO_NONE, G_PRIORITY_LOW, NULL, on_children_enumerated, task);
}

void lr_app_create_tree(LRApp *app) {	
    GHashTable *cache;
    
	app->root = g_file_new_for_path("/");
	app->combo_store = gtk_tree_store_new(NUM_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	cache = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
    g_object_set_data_full(G_OBJECT(app->combo_store), "icon-cache", cache, (GDestroyNotify)g_hash_table_destroy);

    start_async_folder_read(app->combo_store, app->root, NULL, cache);
}

int main(int argc, char *argv[]) {
	LRApp app;
	GtkWidget *window;
	GtkWidget *vbox;
  	GtkWidget *menubar;
  	GtkWidget *toolbar;
	GtkWidget *toolbar2;
	GtkWidget *view;
	GtkWidget *statusbar;
	GtkWidget *s1;
	GtkWidget *s2;
	GtkWidget *s3;
	GtkWidget *btn;
	
	gtk_init(&argc, &argv);
	
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);
	gtk_window_set_title(GTK_WINDOW(window), "PBRAR");
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	menubar = gtk_menu_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
	lr_create_menus(menubar);
	
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_LARGE_TOOLBAR); 
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_style_context_add_class(gtk_widget_get_style_context(toolbar), GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	lr_create_tool_items(GTK_TOOLBAR(toolbar));
	
	toolbar2 = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar2), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar2), GTK_ICON_SIZE_SMALL_TOOLBAR); 
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_style_context_add_class(gtk_widget_get_style_context(toolbar2), GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), toolbar2, FALSE, FALSE, 0);
	lr_create_secondary_tool_items(GTK_TOOLBAR(toolbar2));
	
	view = create_view();
	statusbar = create_statusbar();
	gtk_box_pack_end(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), view, TRUE, TRUE, 0);
	
	gtk_widget_show_all(window);
	gtk_main();
		
	return 0;
}
