#include <gtk/gtk.h>

enum {    
  COL_DISPLAY_NAME,
  COL_PIXBUF,
  NUM_COLS
};

GtkWidget *create_page1(void) {
    GtkWidget *vbox;
    GtkWidget *frame;
    GtkWidget *vbox2;
	GtkWidget *widget;

    vbox = gtk_vbox_new(FALSE, 18);
    
    widget = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(widget), "<big><b>Welcome to the Imaging and Scanning Wizard</b></big>");
    gtk_label_set_line_wrap(GTK_LABEL(widget), TRUE);
    gtk_misc_set_alignment(GTK_MISC(widget), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);

    widget = gtk_label_new("This wizard helps you import images and scan documents using digital cameras, webcams and scanners.");
    gtk_label_set_line_wrap(GTK_LABEL(widget), TRUE);
	gtk_misc_set_alignment(GTK_MISC(widget), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);

	widget = gtk_radio_button_new_with_label(NULL, "I want to import photos or video frames from a digital photo or video camera");
    gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
    
	widget = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(widget), "I want to take new photos using a digital camera or webcam");
    gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
    
	widget = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(widget), "I want to scan documents using a scanner.");
    gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
    
    gtk_widget_show_all(vbox);
	return vbox;
}

GtkTreeModel *init_model(void) {
	GtkListStore *list_store;
	GtkIconTheme *icon_theme;
	GdkPixbuf *image;
	GtkTreeIter iter;

	icon_theme = gtk_icon_theme_get_default();
	list_store = gtk_list_store_new(NUM_COLS, G_TYPE_STRING, GDK_TYPE_PIXBUF);
      
    image = gtk_icon_theme_load_icon(icon_theme, "camera-photo", 32, GTK_ICON_LOOKUP_FORCE_REGULAR, NULL);
	gtk_list_store_append(list_store, &iter);
	gtk_list_store_set(list_store, &iter, COL_DISPLAY_NAME,  "Nikodak SC3000 Digicam", COL_PIXBUF, image, -1);
  	g_object_unref(image);

    image = gtk_icon_theme_load_icon(icon_theme, "camera-web", 32, GTK_ICON_LOOKUP_FORCE_REGULAR, NULL);
	gtk_list_store_append(list_store, &iter);
	gtk_list_store_set(list_store, &iter, COL_DISPLAY_NAME,  "Michealsoft LifeView VX Webcam", COL_PIXBUF, image, -1);
  	g_object_unref(image);

    image = gtk_icon_theme_load_icon(icon_theme, "camera-video", 32, GTK_ICON_LOOKUP_FORCE_REGULAR, NULL);
	gtk_list_store_append(list_store, &iter);
	gtk_list_store_set(list_store, &iter, COL_DISPLAY_NAME,  "Sonii Handikam CMOS-TV921 DV Camera", COL_PIXBUF, image, -1);
  	g_object_unref(image);

    image = gtk_icon_theme_load_icon(icon_theme, "scanner", 32, GTK_ICON_LOOKUP_FORCE_REGULAR, NULL);
	gtk_list_store_append(list_store, &iter);
	gtk_list_store_set(list_store, &iter, COL_DISPLAY_NAME,  "Space Dynamics ArScan Flatbed (FOX MCCLOUD USES THIS TO FIGHT HIS RIVAL WOLF O DONNELL IN PAPERWORK!!!)", COL_PIXBUF, image, -1);
  	g_object_unref(image);

	return GTK_TREE_MODEL(list_store);
}


GtkWidget *create_page1a(void) {
    GtkWidget *vbox;
    GtkWidget *frame;
    GtkWidget *vbox2;
	GtkWidget *widget;
	GtkWidget *widget2;

    vbox = gtk_vbox_new(FALSE, 18);
    
    widget = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(widget), "<big><b>Welcome to the Imaging and Scanning Wizard</b></big>");
    gtk_label_set_line_wrap(GTK_LABEL(widget), TRUE);
    gtk_misc_set_alignment(GTK_MISC(widget), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
    
    widget = gtk_label_new("This wizard helps you import images and scan documents using digital cameras, webcams and scanners. Use the list below to select your prefered device.");
    gtk_label_set_line_wrap(GTK_LABEL(widget), TRUE);
	gtk_misc_set_alignment(GTK_MISC(widget), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
    
	frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

    vbox2 = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox2);

	widget = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(vbox2), widget, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(widget), GTK_SHADOW_NONE);

	widget2 = gtk_icon_view_new_with_model(init_model());
	gtk_widget_set_size_request(widget, 75, 60);
	gtk_container_add(GTK_CONTAINER(widget), widget2);
	gtk_icon_view_set_text_column(GTK_ICON_VIEW(widget2), COL_DISPLAY_NAME);
	gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(widget2), COL_PIXBUF);
	gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(widget2), GTK_SELECTION_SINGLE);

	widget = gtk_action_bar_new();
	gtk_action_bar_pack_start(GTK_ACTION_BAR(widget), gtk_button_new_from_stock(GTK_STOCK_REFRESH));
    gtk_box_pack_end(GTK_BOX(vbox2), widget, FALSE, FALSE, 0);


    gtk_widget_show_all(vbox);
	return vbox;
}

gboolean draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data) {
	cairo_set_source_rgb(cr, 1,1,1);
    cairo_rectangle(cr, 0, 0, gtk_widget_get_allocated_width(widget), gtk_widget_get_allocated_height(widget));
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 8);
    cairo_move_to(cr, 12, 12);
    cairo_show_text(cr, "Document preview with crop draggers and other elements go here.");

	return FALSE;
}

GtkWidget *create_page2scan(void) {
    GtkWidget *vbox;
    GtkWidget *frame;
    GtkWidget *hbox;
	GtkWidget *widget;
	GtkWidget *widget2;
	GtkWidget *widget3;
	GtkWidget *vbox2;
    GtkWidget *grid;
	GtkIconTheme *icon_theme;
	GdkPixbuf *image;

	icon_theme = gtk_icon_theme_get_default();
    vbox = gtk_vbox_new(FALSE, 18);
    
    widget = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(widget), "<big><b>Choose Scanning Options</b></big>");
    gtk_label_set_line_wrap(GTK_LABEL(widget), TRUE);
    gtk_misc_set_alignment(GTK_MISC(widget), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
    
    widget = gtk_label_new("Choose your desired options. Use the preview function to see which set of options are ideal.");
    gtk_label_set_line_wrap(GTK_LABEL(widget), TRUE);
	gtk_misc_set_alignment(GTK_MISC(widget), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
    
    hbox = gtk_hbox_new(FALSE, 15);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_label_align(GTK_FRAME(frame), 0.04, 0.5);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);

	grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
	gtk_container_set_border_width(GTK_CONTAINER(grid), 8);
	gtk_container_add(GTK_CONTAINER(frame), grid);

    image = gtk_icon_theme_load_icon(icon_theme, "image-x-generic", 32, GTK_ICON_LOOKUP_FORCE_REGULAR, NULL);
	widget = gtk_image_new_from_pixbuf(image);
	gdk_pixbuf_unref(image);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 0, 1, 1);
	
	widget2 = gtk_radio_button_new_with_label(NULL, "Color picture");
	gtk_grid_attach_next_to(GTK_GRID(grid), widget2, widget, GTK_POS_RIGHT, 1, 1);

    image = gdk_pixbuf_copy(image);
	gdk_pixbuf_saturate_and_pixelate(image, image, 0.0, FALSE);
	widget = gtk_image_new_from_pixbuf(image);
	gdk_pixbuf_unref(image);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 1, 1, 1);
	
	widget2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(widget2), "Grayscale picture");
	gtk_grid_attach_next_to(GTK_GRID(grid), widget2, widget, GTK_POS_RIGHT, 1, 1);


    image = gtk_icon_theme_load_icon(icon_theme, "x-office-document", 32, GTK_ICON_LOOKUP_FORCE_REGULAR, NULL);
	widget = gtk_image_new_from_pixbuf(image);
	gdk_pixbuf_unref(image);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 2, 1, 1);
	
	widget2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(widget2), "Black and white picture or text document");
	gtk_grid_attach_next_to(GTK_GRID(grid), widget2, widget, GTK_POS_RIGHT, 1, 1);

    image = gtk_icon_theme_load_icon(icon_theme, "preferences-system", 32, GTK_ICON_LOOKUP_FORCE_REGULAR, NULL);
	widget = gtk_image_new_from_pixbuf(image);
	gdk_pixbuf_unref(image);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 3, 1, 1);
	
	widget2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(widget2), "Custom");
	gtk_grid_attach_next_to(GTK_GRID(grid), widget2, widget, GTK_POS_RIGHT, 1, 1);

	widget2 = gtk_button_new_with_label("Custom settings");
	widget = gtk_image_new_from_icon_name("preferences-system", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(widget2), widget);
	gtk_button_set_image_position(GTK_BUTTON(widget2), GTK_POS_LEFT);
	gtk_button_set_always_show_image(GTK_BUTTON(widget2), TRUE);
	gtk_grid_attach(GTK_GRID(grid), widget2, 0, 4, 2, 1);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);

    vbox2 = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox2);

	widget = gtk_drawing_area_new();
	gtk_widget_set_size_request(widget, 120, 170);
    gtk_box_pack_start(GTK_BOX(vbox2), widget, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(widget), "draw", G_CALLBACK(draw_callback), NULL);

    widget2 = gtk_action_bar_new();
  	gtk_widget_set_size_request(widget2, 300, -1);
	gtk_box_pack_end(GTK_BOX(vbox2), widget2, FALSE, FALSE, 0);

	widget = gtk_button_new_from_stock(GTK_STOCK_REFRESH);
	gtk_action_bar_pack_start(GTK_ACTION_BAR(widget2), widget);

	widget = gtk_button_new_with_label("Enlarge");
	widget3 = gtk_image_new_from_stock(GTK_STOCK_FULLSCREEN, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(widget), widget3);
	gtk_button_set_image_position(GTK_BUTTON(widget), GTK_POS_LEFT);
	gtk_button_set_always_show_image(GTK_BUTTON(widget), TRUE);
	gtk_action_bar_pack_start(GTK_ACTION_BAR(widget2), widget);
	
	widget = gtk_button_new_with_label("Shrink");
	widget3 = gtk_image_new_from_stock(GTK_STOCK_LEAVE_FULLSCREEN, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(widget), widget3);
	gtk_button_set_image_position(GTK_BUTTON(widget), GTK_POS_LEFT);
	gtk_button_set_always_show_image(GTK_BUTTON(widget), TRUE);
	gtk_action_bar_pack_start(GTK_ACTION_BAR(widget2), widget);


    gtk_widget_show_all(vbox);
	return vbox;
}


int main(int argc, char *argv[]) {
	GtkWidget *assistant;
	GtkWidget *page;
	
    gtk_init(&argc, &argv);

    assistant = gtk_assistant_new();
	gtk_window_set_title(GTK_WINDOW(assistant), "Imaging and Scanning Wizard");
    g_signal_connect(assistant, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	if (0) {
		page = create_page1();
		gtk_assistant_append_page(GTK_ASSISTANT(assistant), page);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page, "Task Selection");
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page, GTK_ASSISTANT_PAGE_INTRO);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page, TRUE);
	} else {
		page = create_page1a();
		gtk_assistant_append_page(GTK_ASSISTANT(assistant), page);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page, "Device Selection");
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page, GTK_ASSISTANT_PAGE_INTRO);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page, TRUE);
	}
	
	page = create_page2scan();
	gtk_assistant_append_page(GTK_ASSISTANT(assistant), page);
	gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page, "Setup and Preview");
	gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page, TRUE);

	gtk_window_resize(GTK_WINDOW(assistant), 1, 1);
    gtk_widget_show_all(assistant);
    gtk_main();

    return 0;
}
