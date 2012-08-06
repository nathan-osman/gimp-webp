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
#include <libgimpbase/gimpbase.h>
#include <webp/encode.h>

#include "export-dialog.h"

/* Map of WebP preset values to text descriptions. */
typedef struct {
    WebPPreset id;
    const gchar * description;
} WebPPresetDescription;

WebPPresetDescription preset_map[] = {
    { WEBP_PRESET_DEFAULT, "Default" },
    { WEBP_PRESET_PICTURE, "Picture" },
    { WEBP_PRESET_PHOTO,   "Photo"   },
    { WEBP_PRESET_DRAWING, "Drawing" },
    { WEBP_PRESET_ICON,    "Icon"    },
    { WEBP_PRESET_TEXT,    "Text"    },
    { 0, NULL }
};

/* Stores values from the input controls. */
struct webp_data {
    GtkObject         * quality_scale;
    GtkWidget         * lossless;
    float             * quality;
    WebPEncodingFlags * flags;
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
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->lossless)) == TRUE)
            *(data->flags) |= WEBP_OPTIONS_LOSSLESS;
    }

    /* Quit the loop. */
    gtk_main_quit();
}

/* Utility method for filling preset list with values. */
void fill_preset_list(GtkComboBoxText * preset)
{
    /* Loop through the preset map until we reach an entry
       with a NULL description. */
    WebPPresetDescription * i = preset_map;
    while(i->description)
    {
        gtk_combo_box_text_append_text(preset, i->description);
        ++i;
    }
    
    /* Activate the first item in the list (default). */
    gtk_combo_box_set_active(GTK_COMBO_BOX(preset), 0);
}

/* Event handler for toggling the enabled state of the scale
   when the associated lossless checkbox is checked. */
void on_lossless_toggled(GtkWidget * lossless,
                         gpointer user_data)
{
    gboolean enabled = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lossless));
    
    /* Enable / disable the spinbox based on the checkbox. */
    gimp_scale_entry_set_sensitive(GTK_OBJECT(user_data), enabled);
}

/* Utility method for creating the widgets for the color / alpha channels. */
void create_channel_widgets(const gchar * label_text,
                            GtkBox * parent,
                            GtkWidget ** lossless,
                            GtkObject ** scale)
{
    GtkWidget * hbox;
    GtkWidget * alignment;
    GtkWidget * vbox;
    GtkWidget * label;
    GtkWidget * table;
    
    /* Create the label. */
    label = gtk_label_new(label_text);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_box_pack_start(parent, label, FALSE, FALSE, 0);
    gtk_widget_show(label);
    
    /* Create the horizontal box. */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(parent, hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);
    
    /* Insert the spacer. */
    alignment = gtk_alignment_new(0.0f, 0.0f, 1.0f, 1.0f);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 10);
    gtk_widget_show(alignment);
    
    /* Create the vertical box. */
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
    gtk_widget_show(vbox);
    
    /* Create the checkbox. */
    *lossless = gtk_check_button_new_with_label("Lossless");
    gtk_box_pack_start(GTK_BOX(vbox), *lossless, FALSE, FALSE, 0);
    gtk_widget_show(*lossless);

    /* Create the table. */
    table = gtk_table_new(1, 3, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
    gtk_widget_show(table);

    /* Create the scale. */
    *scale = gimp_scale_entry_new(GTK_TABLE(table), 0, 0,
                                  "Quality:",
                                  150, 0,
                                  90.0f, 0.0f, 100.0f, 1.0f, 10.0f,
                                  0, TRUE, 0.0f, 0.0f,
                                  "Quality for encoding the channel",
                                  NULL);
    
    /* Toggle the scale when the lossless checkbox is toggled. */
    g_signal_connect(*lossless, "toggled", G_CALLBACK(on_lossless_toggled), *scale);
}

/* Displays the dialog. */
int export_dialog(float * quality, WebPEncodingFlags * flags)
{
    struct webp_data data;
    GtkWidget * dialog;
    GtkWidget * description_label;
    GtkWidget * separator;
    GtkWidget * preset_box;
    GtkWidget * preset_label;
    GtkWidget * preset;
    
    /* Color channel options. */
    GtkWidget * color_lossless;
    GtkObject * color_scale;
    
    /* Alpha channel options. */
    GtkWidget * alpha_lossless;
    GtkObject * alpha_scale;
    
    gint response;

    /* Create the dialog - using the new export dialog for Gimp 2.7+
       and falling back to a standard dialog for < Gimp 2.7. */
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
    
    /* Set the window border to 10 pixels and the spacing to 15 pixels. */
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);
    gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog)->vbox), 15);
    
    /* Create the description label. */
    description_label = gtk_label_new("The options below allow you to customize\nthe WebP image that is created.");
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), description_label, FALSE, FALSE, 0);
    gtk_widget_show(description_label);
    
    /* Create the horizontal box and separator for the preset. */
    preset_box = gtk_hbox_new(FALSE, 20);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), preset_box, FALSE, FALSE, 0);
    gtk_widget_show(preset_box);
    
    separator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), separator, FALSE, FALSE, 0);
    gtk_widget_show(separator);
    
    /* Create the preset label. */
    preset_label = gtk_label_new("Preset:");
    gtk_box_pack_start(GTK_BOX(preset_box), preset_label, FALSE, FALSE, 0);
    gtk_widget_show(preset_label);
    
    /* Create the preset combo box. */
    preset = gtk_combo_box_text_new();
    gtk_box_pack_start(GTK_BOX(preset_box), preset, TRUE, TRUE, 0);
    gtk_widget_show(preset);
    
    /* Add some presets. */
    fill_preset_list(GTK_COMBO_BOX_TEXT(preset));
    
    /* Create the controls for the channel options. */
    create_channel_widgets("Color channels:", GTK_BOX(GTK_DIALOG(dialog)->vbox), &color_lossless, &color_scale);
    create_channel_widgets("Alpha channel:", GTK_BOX(GTK_DIALOG(dialog)->vbox), &alpha_lossless, &alpha_scale);

    /* Connect to the response signal. */
    data.quality_scale = color_scale;
    data.lossless      = color_lossless;
    data.quality       = quality;
    data.flags         = flags;

    g_signal_connect(dialog, "response", G_CALLBACK(on_response),   &data);
    g_signal_connect(dialog, "destroy",  G_CALLBACK(gtk_main_quit), NULL);

    /* Show the dialog and run it. */
    gtk_widget_show(dialog);
    response = gimp_dialog_run(GIMP_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return response == GTK_RESPONSE_OK;
}
