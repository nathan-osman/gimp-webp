/**
 * gimp-webp - WebP Plugin for the GIMP
 * Copyright (C) 2015  Nathan Osman
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimpbase/gimpbase.h>
#include <string.h>

#include "export-dialog.h"
#include "file-webp.h"

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
    { -1, NULL }
};

/* Storage for grabbing data from the controls and populating the config. */
typedef struct {
    int          response;
    GtkWidget  * dialog;
    GtkWidget  * preset;
    GtkObject  * preset_quality;
    GtkWidget  * custom_vbox;
    GtkWidget  * color_lossless;
    GtkObject  * color_quality;
    GtkWidget  * alpha_lossless;
    GtkObject  * alpha_quality;
    WebPConfig * config;
} WebPControls;

/* Utility method for obtaining a WebPPreset given its descriptive name.
   Returns -1 if the name was not found. */
WebPPreset get_webp_preset_from_description(const gchar * description)
{
    /* Loop through the map, comparing the strings. */
    WebPPresetDescription * i = preset_map;
    while(i->description && strcmp(i->description, description))
        ++i;

    /* Either we found a match or we've reached the
       terminating entry in the map. */
    return i->id;
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

    /* Insert "custom" item. */
    gtk_combo_box_text_append_text(preset, "Custom");

    /* Activate the first item in the list ("Default"). */
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

/* Utility method for creating scale (and optionally lossless checkbox). */
void create_quality_widgets(const gchar * label_text,
                            GtkBox * parent,
                            GtkWidget ** lossless_radio,
                            GtkObject ** lossy_quality)
{
    GtkWidget * label;
    GtkWidget * hbox,
              * vbox;
    GtkWidget * alignment;
    GtkWidget * table;

    /* Create the label if requested. */
    if(label_text)
    {
        label = gtk_label_new(label_text);
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
        gtk_box_pack_start(parent, label, FALSE, FALSE, 0);
        gtk_widget_show(label);
    }

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
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
    gtk_widget_show(vbox);

    /* Create the table and scale, but don't pack it yet. */
    table = gtk_table_new(1, 3, FALSE);
    gtk_widget_show(table);
    *lossy_quality = gimp_scale_entry_new(GTK_TABLE(table), 0, 0,
                                          "Quality:",
                                          150, 0,
                                          90.0f, 0.0f, 100.0f, 1.0f, 10.0f,
                                          0, TRUE, 0.0f, 0.0f,
                                          "Quality for encoding the channel",
                                          NULL);

    if(lossless_radio)
    {
        GtkWidget * lossy_radio;
        GtkWidget * lossy_hbox;

        /* Create the lossless radio button. */
        *lossless_radio =
          gtk_radio_button_new_with_label(NULL, "Lossless");
        gtk_box_pack_start(GTK_BOX(vbox), *lossless_radio, FALSE, FALSE, 0);
        gtk_widget_show(*lossless_radio);

        /* Create the lossy hbox. */
        lossy_hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), lossy_hbox, FALSE, FALSE, 0);
        gtk_widget_show(lossy_hbox);

        /* Create the lossy radio button and set it as toogled. */
        lossy_radio =
          gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(*lossless_radio), "Lossy - ");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lossy_radio), TRUE);
        gtk_box_pack_start(GTK_BOX(lossy_hbox), lossy_radio, FALSE, FALSE, 0);
        gtk_widget_show(lossy_radio);

        /* When the lossy radio button is toggled, we disable the scale. */
        g_signal_connect(*lossless_radio, "toggled", G_CALLBACK(on_lossless_toggled), *lossy_quality);

        /* Pack the scale into the lossy radio button. */
        gtk_box_pack_start(GTK_BOX(lossy_hbox), table, FALSE, FALSE, 0);
    }
    else
        /* Pack the scale into the vbox. */
        gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
}

/* Handler for showing / hiding the custom settings when the preset is changed. */
void on_preset_changed(GtkWidget * preset,
                       WebPControls * controls)
{
    /* Determine which preset the user has selected (if any). */
    gchar * selection = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(preset));

    /* Look through our map to try to find a match. */
    WebPPreset selected_preset = get_webp_preset_from_description(selection);
    g_free(selection);

    /* Enable controls based on if "custom" is selected. */
    gtk_widget_set_visible(controls->custom_vbox, selected_preset == -1);
    gimp_scale_entry_set_sensitive(GTK_OBJECT(controls->preset_quality), selected_preset != -1);

    /* Recalculate the size of the dialog (there ought to be an easier way...). */
    gtk_window_reshow_with_initial_size(GTK_WINDOW(controls->dialog));
}

/* Handler for accepting or rejecting the dialog box. */
void on_dialog_response(GtkWidget * dialog,
                        gint response_id,
                        WebPControls * controls)
{
    controls->response = response_id;

    /* Only do anything if the user has accepted the dialog. */
    if(response_id == GTK_RESPONSE_OK)
    {
        /* Determine which preset the user has selected (if any). */
        gchar * selection = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(controls->preset));

        /* Look through our map to try to find a match. */
        WebPPreset selected_preset = get_webp_preset_from_description(selection);
        g_free(selection);

        /* Check to see if we found one. */
        if(selected_preset != -1)
        {
            /* Load the preset. */
            /* TODO: check for error here. */
            WebPConfigPreset(controls->config,
                             selected_preset,
                             gtk_range_get_value(GTK_RANGE(GIMP_SCALE_ENTRY_SCALE(controls->preset_quality))));
        }
        else
        {
            /* TODO: check for error here too. */
            WebPConfigInit(controls->config);

            /* Otherwise, the user has selected the custom preset (which... really isn't
               a preset :P) Fetch the lossless and quality values for the color channel. */
            controls->config->lossless =
              gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controls->color_lossless));
            controls->config->quality =
              gtk_range_get_value(GTK_RANGE(GIMP_SCALE_ENTRY_SCALE(controls->color_quality)));

            /* Do the same thing for the alpha channel. */
            controls->config->alpha_quality =
              gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controls->alpha_lossless))?
              100:gtk_range_get_value(GTK_RANGE(GIMP_SCALE_ENTRY_SCALE(controls->alpha_quality)));
        }
    }

    gtk_widget_destroy(dialog);
}

/* Displays the dialog. */
int export_dialog(WebPConfig * config)
{
    GtkWidget * description_label;
    GtkWidget * preset_box;
    GtkWidget * preset_label;
    GtkWidget * separator;
    WebPControls controls;

    controls.config = config;

    /* Create the dialog - using the new export dialog for Gimp 2.7+
       and falling back to a standard dialog for < Gimp 2.7. */
#if (GIMP_MAJOR_VERSION == 2 && GIMP_MINOR_VERSION >= 7) || GIMP_MAJOR_VERSION > 2
    controls.dialog = gimp_export_dialog_new("WebP",
                                             BINARY_NAME,
                                             SAVE_PROCEDURE);
#else
    controls.dialog = gimp_dialog_new("WebP",
                                      BINARY_NAME,
                                      NULL, 0,
                                      NULL,
                                      SAVE_PROCEDURE,
                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                      GTK_STOCK_SAVE,   GTK_RESPONSE_OK,
                                      NULL);
#endif

    /* Set the window border to 10 pixels and the spacing to 5 pixels. */
    gtk_container_set_border_width(GTK_CONTAINER(controls.dialog), 10);
    gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(controls.dialog)->vbox), 5);

    /* Create the description label. */
    description_label = gtk_label_new("The options below allow you to customize the WebP image\nthat is created.");
    gtk_misc_set_alignment(GTK_MISC(description_label), 0, 0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(controls.dialog)->vbox), description_label, FALSE, FALSE, 0);
    gtk_widget_show(description_label);

    /* Create the horizontal box for the preset. */
    preset_box = gtk_hbox_new(FALSE, 20);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(controls.dialog)->vbox), preset_box, FALSE, FALSE, 0);
    gtk_widget_show(preset_box);

    /* Create the preset label. */
    preset_label = gtk_label_new("Preset:");
    gtk_box_pack_start(GTK_BOX(preset_box), preset_label, FALSE, FALSE, 0);
    gtk_widget_show(preset_label);

    /* Create the preset combo box. */
    controls.preset = gtk_combo_box_text_new();
    gtk_box_pack_start(GTK_BOX(preset_box), controls.preset, TRUE, TRUE, 0);
    gtk_widget_show(controls.preset);

    /* Add some presets. */
    fill_preset_list(GTK_COMBO_BOX_TEXT(controls.preset));

    /* Create the quality controls for the presets. */
    create_quality_widgets(NULL,
                           GTK_BOX(GTK_DIALOG(controls.dialog)->vbox),
                           NULL,
                           &controls.preset_quality);

    /* Create the vbox for the custom settings. */
    controls.custom_vbox = gtk_vbox_new(FALSE, 10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(controls.dialog)->vbox), controls.custom_vbox, FALSE, FALSE, 0);

    /* Insert a separator. */
    separator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(controls.custom_vbox), separator, FALSE, FALSE, 0);
    gtk_widget_show(separator);

    /* Create the controls for the channel options. */
    create_quality_widgets("Color channels:",
                           GTK_BOX(controls.custom_vbox),
                           &controls.color_lossless,
                           &controls.color_quality);

    create_quality_widgets("Alpha channel:",
                           GTK_BOX(controls.custom_vbox),
                           &controls.alpha_lossless,
                           &controls.alpha_quality);

    /* Connect to the changed signal in the preset list so we can
       enable / disable the controls for custom settings. */
    g_signal_connect(controls.preset, "changed", G_CALLBACK(on_preset_changed), &controls);

    /* We need to store the response in the WebPControls structure
       when the dialog box is accepted. */
    g_signal_connect(controls.dialog, "response", G_CALLBACK(on_dialog_response), &controls);
    g_signal_connect(controls.dialog, "destroy",  G_CALLBACK(gtk_main_quit),      NULL);

    /* Show the dialog and run the main loop. */
    gtk_widget_show(controls.dialog);
    gtk_main();

    return controls.response == GTK_RESPONSE_OK;
}
