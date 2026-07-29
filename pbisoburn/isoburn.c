#include <gtk/gtk.h>
#include <libburn/libburn.h>
#include <libisofs/libisofs.h>
#include <libisoburn/libisoburn.h>

typedef enum {
	PB_ISO_ERROR,
	PB_ISO_SELECT_IMAGE,
	PB_ISO_IMAGE_LOADED,
	PB_ISO_SELECT_ANOTHER_BURNER_OR_INSERT_DISC,
	PB_ISO_READY,
} PBIsoBurnState;

typedef enum {
	PB_ISO_NO_FLAGS = 0,
	PB_ISO_FORCE_FILE_BUTTON = 1,
	PB_ISO_ALTERNATE_BURNER_NAME = 2,
} PBIsoBurnFlags;

typedef struct  {
	GtkWidget *image_file;
	GtkWidget *burner;
	GtkWidget *status_label;
	GtkWidget *burn;
	GtkWidget *cancel;
	GtkWidget *status_vbox;
	GtkWidget *status_bar;
	GtkWidget *verify;	
	gboolean gui_creation_finished;
	
	struct burn_drive_info *drives;
	unsigned int drives_sz;
	int grabbed_drive;
	
	IsoImage *iso;
	const char *iso_to_burn;
    struct burn_drive_info *iso_drive;
	char messages[1024];
	
	const gchar *status;
	PBIsoBurnState state;
} PBIsoBurnApp;

void pb_update_gui_state(PBIsoBurnApp *app) {
	switch (app->state) {
	  case PB_ISO_SELECT_IMAGE:
	  case PB_ISO_ERROR:
		gtk_widget_set_sensitive(app->burn, FALSE);
		gtk_widget_set_sensitive(app->verify, FALSE);
		gtk_widget_set_sensitive(app->burner, FALSE);
		gtk_widget_hide(app->status_bar);
		gtk_widget_set_size_request(app->status_vbox, -1, 150);
		if (!app->iso_to_burn && app->state != PB_ISO_SELECT_IMAGE) {
			gtk_widget_set_sensitive(app->image_file, FALSE);
		}	
		break;
	  case PB_ISO_SELECT_ANOTHER_BURNER_OR_INSERT_DISC:
		gtk_widget_set_sensitive(app->burn, FALSE);
		gtk_widget_set_sensitive(app->verify, FALSE);
		gtk_widget_hide(app->status_bar);
		gtk_widget_set_size_request(app->status_vbox, -1, 150);
	  default:
		break;
	}
}

void pb_deselect_drive(PBIsoBurnApp *app) {
	if (app->grabbed_drive != -1)  {
		burn_drive_release(app->drives[app->grabbed_drive].drive, 0);
	}
}

void pb_select_drive(PBIsoBurnApp *app) {
	char disc_name[80];
	unsigned int index;
	int disc_number;

	if (app->burner) {
		index = gtk_combo_box_get_active(GTK_COMBO_BOX(app->burner));
	} else {
		index = 0;
	}
	
    if (isoburn_drive_grab(app->drives[index].drive, 1) != 1) {
		app->status = "The disc burner is already in use. Make sure no other programs are using the burner, and then try again.";
    }
    
	burn_disc_get_profile(app->drives[index].drive, &disc_number, disc_name);
	if (!disc_number) {
		app->status = "There is no disc in your disc burner. Insert a blank recordable disc, and then try again.";
		app->state = PB_ISO_SELECT_ANOTHER_BURNER_OR_INSERT_DISC;
	} else {
		printf("%d %s\n", disc_number, disc_name);
	}
	
	app->grabbed_drive = index;
	
	gtk_label_set_text(GTK_LABEL(app->status_label), app->status);
	pb_update_gui_state(app);
}

void pb_burner_changed(GtkComboBox *self, gpointer user_data) {
	PBIsoBurnApp *app;
	
	app = (PBIsoBurnApp *)user_data;
	if (app->gui_creation_finished) {
		pb_deselect_drive(app);
		pb_select_drive(app);
	}
}

void pb_create_gui(PBIsoBurnApp *app) {
	GtkWidget *window;
	GtkWidget *table;
	GtkWidget *image_file_label;
	GtkWidget *burner_label;
	GtkWidget *frame;
	GtkWidget *button_box;
	GtkWidget *icon;
	GtkFileFilter *filter;
	unsigned int i;
	
	app->gui_creation_finished = FALSE;
	
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), -1, -1);
	gtk_window_set_title(GTK_WINDOW(window), "Disc Image Burner");
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(window), 12);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), G_OBJECT(window));
	
	table = gtk_table_new(5, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table), 15);
	gtk_table_set_row_spacings(GTK_TABLE(table), 15);
	gtk_container_add(GTK_CONTAINER(window), table);

	image_file_label = gtk_label_new("Disc image file:");
	gtk_misc_set_alignment(GTK_MISC(image_file_label), 0, 0.5f);
	gtk_table_attach_defaults(GTK_TABLE(table), image_file_label, 0, 1, 0, 1);
	
	if (app->iso_to_burn) {
		app->image_file = gtk_label_new(g_basename(app->iso_to_burn));
		gtk_label_set_ellipsize(GTK_LABEL(app->image_file), PANGO_ELLIPSIZE_END);
		gtk_misc_set_alignment(GTK_MISC(app->image_file), 0, 0.5f);
		gtk_table_attach_defaults(GTK_TABLE(table), app->image_file, 1, 2, 0, 1);
	} else {
		app->image_file = gtk_file_chooser_button_new("Select an image file to burn", GTK_FILE_CHOOSER_ACTION_OPEN);
		gtk_table_attach_defaults(GTK_TABLE(table), app->image_file, 1, 2, 0, 1);

		filter = gtk_file_filter_new();
		gtk_file_filter_add_pattern(filter, "*.iso");
		gtk_file_filter_add_pattern(filter, "*.ISO");
		gtk_file_filter_add_mime_type(filter, "application/x-iso9660-image");
		gtk_file_filter_add_mime_type(filter, "application/x-cd-image");
		gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(app->image_file), filter);
	}
	
	burner_label = gtk_label_new_with_mnemonic("_Disc burner:");
	gtk_misc_set_alignment(GTK_MISC(burner_label), 0, 0.5f);
	gtk_table_attach_defaults(GTK_TABLE(table), burner_label, 0, 1, 1, 2);
	
	app->burner = gtk_combo_box_text_new();
	gtk_combo_box_set_active(GTK_COMBO_BOX(app->burner), 0);
	g_signal_connect(G_OBJECT(app->burner), "changed", G_CALLBACK(pb_burner_changed), app);
	gtk_table_attach_defaults(GTK_TABLE(table), app->burner, 1, 2, 1, 2);
	
    for (i = 0; i < app->drives_sz; i++) {
		const gchar *type_text;
		gchar *text;
		char adr[BURN_DRIVE_ADR_LEN];
		
		burn_drive_d_get_adr(app->drives[i].drive, adr);

		/*if (app->drives[i].write_cdr && !app->drives[i].write_cdrw && !app->drives[i].write_dvdr && !app->drives[i].write_dvdram)  {
			type_text = "CD-R Drive";
		} else if (app->drives[i].write_cdrw && !app->drives[i].write_dvdr && !app->drives[i].write_dvdram)  {
			type_text = "CD-RW Drive";
		} else if (!app->drives[i].write_cdr && !app->drives[i].write_cdrw && !app->drives[i].write_dvdram && app->drives[i].write_dvdr)  {
			type_text = "DVD R Drive";
		} else if (app->drives[i].write_cdr && !app->drives[i].write_cdrw && !app->drives[i].write_dvdram && app->drives[i].write_dvdr)  {
			type_text = "DVD/CD-R Drive";
		} else if (app->drives[i].write_cdrw && !app->drives[i].write_dvdram && app->drives[i].write_dvdr)  {
			type_text = "DVD/CD-RW Drive";
		} else if (app->drives[i].write_cdr && app->drives[i].write_cdrw && app->drives[i].write_dvdr && app->drives[i].write_dvdram) {
			type_text = "DVD/CD-RW Drive";
		} else if (app->drives[i].write_dvdram)  {
			type_text = "DVD RAM Drive";
		}*/
	
		/*text = g_strconcat(type_text, " (", adr, ")", NULL);*/
		text = g_strconcat(app->drives[i].vendor, " ", app->drives[i].product, " ", app->drives[i].revision, " (", adr, ")", NULL);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->burner), text);
    }

	if (app->drives_sz) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(app->burner), 0);
	}

	frame = gtk_frame_new("Status");
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_frame_set_label_align(GTK_FRAME(frame), 0.03, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 2, 2, 3);
	
	app->status_vbox = gtk_vbox_new(FALSE, 15);
	gtk_container_set_border_width(GTK_CONTAINER(app->status_vbox), 12);
	gtk_container_add(GTK_CONTAINER(frame), app->status_vbox);

	app->status_label = gtk_label_new(app->status);
	gtk_widget_set_size_request(app->status_label, 300, -1);
	gtk_label_set_line_wrap(GTK_LABEL(app->status_label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(app->status_label), 0, 0.5f);
	gtk_box_pack_start(GTK_BOX(app->status_vbox), app->status_label, FALSE, FALSE, 0);

	app->status_bar = gtk_progress_bar_new();
	gtk_box_pack_end(GTK_BOX(app->status_vbox), app->status_bar, FALSE, FALSE, 20);

	app->verify = gtk_check_button_new_with_mnemonic("_Verify disc after burning");
	gtk_table_attach_defaults(GTK_TABLE(table), app->verify, 0, 2, 3, 4);

	button_box = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(button_box), 8);
  	gtk_table_attach_defaults(GTK_TABLE(table), button_box, 0, 2, 4, 5);

	icon = gtk_image_new_from_stock(GTK_STOCK_CDROM, GTK_ICON_SIZE_BUTTON);
	app->burn = gtk_button_new_with_mnemonic("_Burn");
	gtk_button_set_image(GTK_BUTTON(app->burn), icon);
	gtk_container_add(GTK_CONTAINER(button_box), app->burn);
	
	app->cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);	
	g_signal_connect(G_OBJECT(app->cancel), "clicked", G_CALLBACK(gtk_main_quit), G_OBJECT(window));
	gtk_container_add(GTK_CONTAINER(button_box), app->cancel);
	
	gtk_widget_show_all(window);
	pb_update_gui_state(app);
	app->gui_creation_finished = TRUE;
}

void pb_read_iso(PBIsoBurnApp *app) {
	struct isoburn_read_opts *ropts;
	gchar *stdio_path;
	#define INVALID_MESSAGE "The selected disc image file isn't valid."
	
	stdio_path = g_strconcat("stdio:/", app->iso_to_burn, NULL);
	if (isoburn_drive_scan_and_grab(&app->iso_drive, stdio_path, 0) <= 0) {
		app->status = INVALID_MESSAGE;
	}
	g_free(stdio_path);

	isoburn_ropt_new(&ropts, 0);
    if (isoburn_read_image(app->iso_drive->drive, ropts, &app->iso) <= 0) {
		app->status = INVALID_MESSAGE;
    }
    isoburn_ropt_destroy(&ropts, 0);
    isoburn_drive_release(app->iso_drive->drive, 0);
    burn_drive_info_free(app->iso_drive);
    
    app->state = PB_ISO_IMAGE_LOADED;
}

int main(int argc, char *argv[]) {
	PBIsoBurnApp app;
	unsigned int drives_sz;
	int stat;
	
    isoburn_initialize(app.messages, 0);
	gtk_init(&argc, &argv);

	app.grabbed_drive = -1;
	app.burner = NULL;
	app.iso_to_burn = NULL;
	if (argc > 1) {
		if (g_file_test(argv[1], G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS)) {
			gchar *type, *mime;
			
			type = g_content_type_guess(argv[1], NULL, 0, NULL);
			mime = g_content_type_get_mime_type(type);
			
			if (g_strrstr(mime, "x-cd-image") || g_strrstr(mime, "x-iso9660-image")) {
				app.iso_to_burn = argv[1];
			}
			
			g_free(type);
			g_free(mime);
		}
	}

	app.status = NULL;
	app.state = PB_ISO_ERROR;
	app.drives = NULL;
	app.drives_sz = 0;
	
	stat = 0;
	while (!stat) {
		stat = burn_drive_scan(&app.drives, &app.drives_sz);
	}
	
	if (stat < 0) {
		app.status = "An error has occured while enumerating burners. Make sure that a burner is installed properly, and you have the appropriate permissions to burn a disc.";
	}
	
	if (app.drives_sz < 1 && stat >= 0) {
		app.status = "A disc burner wasn't found. Make sure that a burner is installed properly, and you have the appropriate permissions to burn a disc.";
	} 
    
   	if (app.drives_sz >= 1 && stat >= 0) {
		if (app.iso_to_burn) {
			pb_read_iso(&app);
		} else {
			app.status = "Please select a disc image to burn.";
			app.state = PB_ISO_SELECT_IMAGE;
		}
	} 

	pb_create_gui(&app);
	
	if (app.state == PB_ISO_IMAGE_LOADED) {
		pb_select_drive(&app);
	}

	gtk_main();

	if (app.drives) {
		burn_drive_info_free(app.drives);
	}
    isoburn_finish();
	return 0;
}
