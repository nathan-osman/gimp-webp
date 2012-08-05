/*=======================================================================
              WebP load / save plugin for the GIMP
                 Copyright 2012 - Nathan Osman

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
=======================================================================*/

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimpbase/gimpversion.h>

#include "export-dialog.h"
#include "file-webp.h"

/* Stores values from the input controls. */
struct webp_data {
    int         response;
    GtkObject * quality_scale;
    GtkWidget * lossless;
    float     * quality;
    int       * flags;
};

/* Handler for accepting or rejecting the dialog box. */
void on_response(GtkDialog * dialog,
                 gint response_id,
                 gpointer user_data)
{
    /* If the user has accepted the dialog, then copy the value of
       the input controls to our structure. */
    if(response_id == GTK_RESPONSE_OK)
    {
        struct webp_data * data = user_data;

        /* Fetch the quality. */
        GtkHScale * hscale = GIMP_SCALE_ENTRY_SCALE(data->quality_scale);
        *(data->quality) = gtk_range_get_value(GTK_RANGE(hscale));

        /* Determine if the lossless checkbox is checked. */
        if(gtk_toggle_button_get_mode(GTK_TOGGLE_BUTTON(data->lossless)) == TRUE)
            *(data->flags) |= WEBP_OPTIONS_LOSSLESS;

        /* Indicate a positive response. */
        data->response = 1;
    }

    /* Quit the loop. */
    gtk_main_quit();
}

int export_dialog(float * quality, int * flags)
{
    struct webp_data data;
    GtkWidget * dialog;
    GtkWidget * vbox;
    GtkWidget * label;
    GtkWidget * table;
    GtkObject * scale;
    GtkWidget * lossless;

    /* Create the dialog - using the new export dialog for Gimp 2.7+
       and falling back to a GTK dialog for < Gimp 2.7. */
#if (GIMP_MAJOR_VERSION == 2 && GIMP_MINOR_VERSION >= 7) || GIMP_MAJOR_VERSION > 2
    dialog = gimp_export_dialog_new("WebP",
                                    BINARY_NAME,
                                    SAVE_PROCEDURE);
#else
    dialog = gimp_dialog_new("WebP",
                             BINARY_NAME,
                             NULL, 0,
                             NULL,
                             SAVE_PROCEDURE,
                             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                             GTK_STOCK_SAVE,   GTK_RESPONSE_OK,
                             NULL);
#endif

    /* Create the VBox. */
    vbox = gtk_vbox_new(TRUE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       vbox, TRUE, TRUE, 2);
    gtk_widget_show(vbox);

    /* Create the label. */
    label = gtk_label_new("The options below allow you to customize\nthe WebP image that is created.");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 2);
    gtk_widget_show(label);

    /* Create the table. */
    table = gtk_table_new(1, 3, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 6);
    gtk_widget_show(table);

    /* Create the scale. */
    scale = gimp_scale_entry_new(GTK_TABLE(table), 0, 0,
                                 "Quality:",
                                 150, 0,
                                 90.0f, 0.0f, 100.0f, 1.0f, 10.0f,
                                 0, TRUE, 0.0f, 0.0f,
                                 "Quality for encoding the image",
                                 NULL);

    /* Create the checkbox. */
    lossless = gtk_check_button_new_with_label("Lossless");
    gtk_box_pack_start(GTK_BOX(vbox), lossless, FALSE, FALSE, 6);
    gtk_widget_show(lossless);

    // Connect to the response signal
    data.response      = 0;
    data.quality_scale = scale;
    data.lossless      = lossless;
    data.quality       = quality;
    data.flags         = flags;

    g_signal_connect(dialog, "response", G_CALLBACK(on_response),   &data);
    g_signal_connect(dialog, "destroy",  G_CALLBACK(gtk_main_quit), NULL);

    // Show the dialog and run it
    gtk_widget_show(dialog);
    gimp_dialog_run(GIMP_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return data.response;
}
