/**
 * gimp-webp - WebP Plugin for the GIMP
 * Copyright (C) 2016  Nathan Osman & Ben Touchette
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

#include "webp-dialog.h"
#include "webp.h"

struct {
    const gchar *id;
    const gchar *label;
} presets[] = {
    { "default", "Default" },
    { "picture", "Picture" },
    { "photo",   "Photo" },
    { "drawing", "Drawing" },
    { "icon",    "Icon" },
    { "text",    "Text" },
    { 0 }
};

void save_dialog_response(GtkWidget *widget,
                          gint       response_id,
                          gpointer   data)
{
    /* Store the response */
    *(GtkResponseType *)data = response_id;

    /* Close the dialog */
    gtk_widget_destroy(widget);
}

GtkListStore *save_dialog_presets()
{
    GtkListStore *list_store;
    int           i;

    /* Create the model */
    list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

    /* Insert the entries */
    for(i = 0; presets[i].id; ++i) {
        gtk_list_store_insert_with_values(list_store,
                                          NULL,
                                          -1,
                                          0, presets[i].id,
                                          1, presets[i].label,
                                          -1);
    }

    return list_store;
}

void save_dialog_set_preset(GtkWidget *widget,
                            gpointer   data)
{
    *(gchar **)data = gimp_string_combo_box_get_active(GIMP_STRING_COMBO_BOX(widget));
}

void save_dialog_toggle_scale(GtkWidget *widget,
                              gpointer   data)
{
    gimp_scale_entry_set_sensitive(GTK_OBJECT(data),
                                   !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

#ifdef WEBP_0_5
void save_dialog_toggle_checkbox(GtkWidget *widget,
                                 gpointer   data)
{
    gtk_widget_set_sensitive(GTK_OBJECT(data),
                             !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}
#endif

GtkResponseType save_dialog(
        WebPSaveParams *params
#ifdef WEBP_0_5
      , gint32 image_ID
      , gint32 nLayers
#endif
)
{
    GtkWidget       *dialog;
    GtkWidget       *vbox;
    GtkWidget       *label;
    GtkWidget       *table;
    GtkWidget       *preset_label;
    GtkListStore    *preset_list;
    GtkWidget       *preset_combo;
    GtkObject       *quality_scale;
    GtkObject       *alpha_quality_scale;
    GtkWidget       *lossless_checkbox;
#ifdef WEBP_0_5
    GtkWidget       *animation_checkbox;
    GtkWidget       *loop_anim_checkbox;
    gboolean         animation_supported = FALSE;
#endif
    GtkResponseType  response;

#ifdef WEBP_0_5
    /* Determine if the image contains more than one layer */
    animation_supported = nLayers > 1;
#endif

    /* Create the dialog */
    dialog = gimp_export_dialog_new("WebP",
                                    BINARY_NAME,
                                    SAVE_PROCEDURE);

    /* Store the response when the dialog is closed */
    g_signal_connect(dialog,
                     "response",
                     G_CALLBACK(save_dialog_response),
                     &response);

    /* Quit the main loop when the dialog is closed */
    g_signal_connect(dialog,
                     "destroy",
                     G_CALLBACK(gtk_main_quit),
                     NULL);

    /* Create the vbox */
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
    gtk_box_pack_start(GTK_BOX(gimp_export_dialog_get_content_area(dialog)),
                       vbox,
                       FALSE, FALSE,
                       0);
    gtk_widget_show(vbox);

    /* Create the descriptive label at the top */
    label = gtk_label_new("Use the options below to customize the image.");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    gtk_widget_show(label);

    /* Create the table */
    table = gtk_table_new(
#ifdef WEBP_0_5
                animation_supported == TRUE ? 6 : 4
#else
                4
#endif
              , 3
              , FALSE
    );
    gtk_table_set_row_spacings(GTK_TABLE(table), 6);
    gtk_table_set_col_spacings(GTK_TABLE(table), 6);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
    gtk_widget_show(table);

    /* Create the label for the selecting a preset */
    preset_label = gtk_label_new("Preset:");
    gtk_table_attach(GTK_TABLE(table),
                     preset_label,
                     0, 1,
                     0, 1,
                     0, 0,
                     0, 0);
    gtk_widget_show(preset_label);

    /* Create the combobox containing the presets */
    preset_list = save_dialog_presets();
    preset_combo = gimp_string_combo_box_new(GTK_TREE_MODEL(preset_list), 0, 1);
    g_object_unref(preset_list);

    gimp_string_combo_box_set_active(GIMP_STRING_COMBO_BOX(preset_combo), params->preset);
    gtk_table_attach(GTK_TABLE(table),
                     preset_combo,
                     1, 3,
                     0, 1,
                     GTK_FILL, GTK_FILL,
                     0, 0);
    gtk_widget_show(preset_combo);

    g_signal_connect(preset_combo, "changed",
                     G_CALLBACK(save_dialog_set_preset),
                     &params->preset);

    /* Create the slider for image quality */
    quality_scale = gimp_scale_entry_new(GTK_TABLE(table),
                                         0, 1,
                                         "Image quality:",
                                         125,
                                         0,
                                         params->quality,
                                         0.0, 100.0,
                                         1.0, 10.0,
                                         0, TRUE,
                                         0.0, 0.0,
                                         "Image quality",
                                         NULL);
    gimp_scale_entry_set_sensitive(quality_scale, !params->lossless);
    g_signal_connect(quality_scale, "value-changed",
                     G_CALLBACK(gimp_float_adjustment_update),
                     &params->quality);

    /* Create the slider for alpha channel quality */
    alpha_quality_scale = gimp_scale_entry_new(GTK_TABLE(table),
                                               0, 2,
                                               "Alpha quality:",
                                               125,
                                               0,
                                               params->alpha_quality,
                                               0.0, 100.0,
                                               1.0, 10.0,
                                               0, TRUE,
                                               0.0, 0.0,
                                               "Alpha channel quality",
                                               NULL);
    gimp_scale_entry_set_sensitive(alpha_quality_scale, !params->lossless);
    g_signal_connect(alpha_quality_scale, "value-changed",
                     G_CALLBACK(gimp_float_adjustment_update),
                     &params->alpha_quality);

    /* Create the lossless checkbox */
    lossless_checkbox = gtk_check_button_new_with_label("Lossless");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lossless_checkbox), params->lossless);
    gtk_table_attach(GTK_TABLE(table),
                     lossless_checkbox,
                     1, 3,
                     3, 4,
                     GTK_FILL, GTK_FILL,
                     0, 0);
    gtk_widget_show(lossless_checkbox);

    g_signal_connect(lossless_checkbox, "toggled",
                     G_CALLBACK(gimp_toggle_button_update),
                     &params->lossless);

    /* Enable and disable the sliders when the lossless option is selected */
    g_signal_connect(lossless_checkbox, "toggled",
                     G_CALLBACK(save_dialog_toggle_scale),
                     quality_scale);
    g_signal_connect(lossless_checkbox, "toggled",
                     G_CALLBACK(save_dialog_toggle_scale),
                     alpha_quality_scale);

#ifdef WEBP_0_5
    if (animation_supported == TRUE) {

        /* Create the animation checkbox */
        animation_checkbox = gtk_check_button_new_with_label("Use animation");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(animation_checkbox), params->animation);
        gtk_table_attach(GTK_TABLE(table),
                         animation_checkbox,
                         1, 3,
                         4, 5,
                         GTK_FILL, GTK_FILL,
                         0, 0);
        gtk_widget_show(animation_checkbox);

        g_signal_connect(animation_checkbox, "toggled",
                         G_CALLBACK(gimp_toggle_button_update),
                         &params->animation);

        /* Create the loop animation checkbox */
        loop_anim_checkbox = gtk_check_button_new_with_label("Loop infinitely");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(loop_anim_checkbox), params->loop);
        gtk_table_attach(GTK_TABLE(table),
                         loop_anim_checkbox,
                         1, 3,
                         5, 6,
                         GTK_FILL, GTK_FILL,
                         0, 0);
        gtk_widget_show(loop_anim_checkbox);

        g_signal_connect(loop_anim_checkbox, "toggled",
                         G_CALLBACK(gimp_toggle_button_update),
                         &params->loop);

        /* Enable and disable the loop checkbox when the animation checkbox is selected */
        g_signal_connect(animation_checkbox, "toggled",
                         G_CALLBACK(save_dialog_toggle_checkbox),
                         loop_anim_checkbox);
    }
#endif

    /* Display the dialog and enter the main event loop */
    gtk_widget_show(dialog);
    gtk_main();

    return response;
}
