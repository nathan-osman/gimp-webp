/*=======================================================================
              WebP load / save plugin for the GIMP
                 Copyright 2011 - Nathan Osman

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

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Gimp and WebP headers
#include <libgimp/gimp.h>
#include <libgimp/gimpexport.h>
#include <libgimp/gimpui.h>

#include <webp/decode.h>
#include <webp/encode.h>

// Procedure names
const char LOAD_PROCEDURE[] = "file-webp-load";
const char SAVE_PROCEDURE[] = "file-webp-save";
const char BINARY_NAME[]    = "file-webp";

// Predeclare our entrypoints
void query();
void run(const gchar *, gint, const GimpParam *, gint *, GimpParam **);

int read_webp(const gchar *);
int write_webp(const gchar *, gint, float);

int save_dialog(float *);

// Declare our plugin entry points
GimpPlugInInfo PLUG_IN_INFO = {
    NULL,
    NULL,
    query,
    run
};

MAIN ()

// The query function
void query()
{
    // Load arguments
    static const GimpParamDef load_arguments[] =
    {
        { GIMP_PDB_INT32,  "run-mode",     "Interactive, non-interactive" },
        { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
        { GIMP_PDB_STRING, "raw-filename", "The name entered" }
    };

    // Load return values
    static const GimpParamDef load_return_values[] =
    {
        { GIMP_PDB_IMAGE, "image", "Output image" }
    };

    // Save arguments
    static const GimpParamDef save_arguments[] =
    {
        { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
        { GIMP_PDB_IMAGE,    "image",        "Input image" },
        { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
        { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
        { GIMP_PDB_STRING,   "raw-filename", "The name entered" },
        { GIMP_PDB_FLOAT,    "quality",      "Quality of the image (0 <= quality <= 100)" }
    };

    // Install the load procedure
    gimp_install_procedure(LOAD_PROCEDURE,
                           "Loads images in the WebP file format",
                           "Loads images in the WebP file format",
                           "Nathan Osman",
                           "Copyright Nathan Osman",
                           "2011",
                           "WebP image",
                           NULL,
                           GIMP_PLUGIN,
                           G_N_ELEMENTS(load_arguments),
                           G_N_ELEMENTS(load_return_values),
                           load_arguments,
                           load_return_values);

    // Install the save procedure
    gimp_install_procedure(SAVE_PROCEDURE,
                           "Saves files in the WebP file format",
                           "Saves files in the WebP file format",
                           "Nathan Osman",
                           "Copyright Nathan Osman",
                           "2011",
                           "WebP image",
                           NULL,
                           GIMP_PLUGIN,
                           G_N_ELEMENTS(save_arguments),
                           0,
                           save_arguments,
                           NULL);

    // Register the load handlers
    gimp_register_file_handler_mime(LOAD_PROCEDURE, "image/webp");
    gimp_register_load_handler(LOAD_PROCEDURE, "webp", "");

    // Now register the save handlers
    gimp_register_file_handler_mime(SAVE_PROCEDURE, "image/webp");
    gimp_register_save_handler(SAVE_PROCEDURE, "webp", "");
}

// The run function
void run(const gchar * name,
         gint nparams,
         const GimpParam * param,
         gint * nreturn_vals,
         GimpParam ** return_vals)
{
    // Create the return value.
    static GimpParam return_values[2];
    GimpExportReturn export;

    *nreturn_vals = 1;
    *return_vals  = return_values;

    // The status
    return_values[0].type          = GIMP_PDB_STATUS;
    return_values[0].data.d_status = GIMP_PDB_SUCCESS;

    // Check to see if this is the load procedure
    if(!strcmp(name, LOAD_PROCEDURE))
    {
        int new_image_id;

        // Check to make sure all parameters were supplied
        if(nparams != 3)
        {
            return_values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
            return;
        }

        // Now read the image
        new_image_id = read_webp(param[1].data.d_string);

        // Check for an error
        if(new_image_id == -1)
        {
            return_values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
            return;
        }

        // Fill in the second return value
        *nreturn_vals = 2;

        return_values[1].type         = GIMP_PDB_IMAGE;
        return_values[1].data.d_image = new_image_id;
    }
    else if(!strcmp(name, SAVE_PROCEDURE))
    {
        gint32 image_id, drawable_id;
        int status;
        float quality;

        // Check to make sure all of the parameters were supplied
        if(nparams != 6)
        {
            return_values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
            return;
        }

        image_id    = param[1].data.d_int32;
        drawable_id = param[2].data.d_int32;

        // Try to export the image
        gimp_ui_init(BINARY_NAME, FALSE);
        export = gimp_export_image(&image_id,
                                   &drawable_id,
                                   "WEBP",
                                   GIMP_EXPORT_CAN_HANDLE_RGB);

        switch(export)
        {
            case GIMP_EXPORT_EXPORT:
            case GIMP_EXPORT_IGNORE:

                // Now get the settings
                if(!save_dialog(&quality))
                {
                    return_values[0].data.d_status = GIMP_PDB_CANCEL;
                    return;
                }

                status = write_webp(param[3].data.d_string,
                                    drawable_id, quality);
                gimp_image_delete(image_id);

                break;
            case GIMP_EXPORT_CANCEL:
                return_values[0].data.d_status = GIMP_PDB_CANCEL;
                return;
        }

        if(!status)
            return_values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
    }
    else
        return_values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
}

// Reads the file from disk
int read_webp(const gchar * filename)
{
    FILE * file;
    long int filesize;
    void * data,
         * image_data;
    int width, height;
    gint32 new_image_id,
           new_layer_id;
    GimpDrawable * drawable;
    GimpPixelRgn rgn;


    // Try to open the file
    file = fopen(filename, "rb");
    if(!file)
        return -1;

    // Get the file size
    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Now prepare a buffer of that size
    // and read the data.
    data = malloc(filesize);
    fread(data, filesize, 1, file);

    // Close the file
    fclose(file);

    // Perform the load procedure and free the raw data.
    image_data = WebPDecodeRGB(data, filesize, &width, &height);
    free(data);

    // Check to make sure that the load was successful
    if(!image_data)
        return -1;

    // Now create the new RGBA image.
    new_image_id = gimp_image_new(width, height, GIMP_RGB);

    // Create the new layer
    new_layer_id = gimp_layer_new(new_image_id,
                                  "Background",
                                  width, height,
                                  GIMP_RGB_IMAGE,
                                  100,
                                  GIMP_NORMAL_MODE);

    // Get the drawable for the layer
    drawable = gimp_drawable_get(new_layer_id);

    // Get a pixel region from the layer
    gimp_pixel_rgn_init(&rgn,
                        drawable,
                        0, 0,
                        width, height,
                        TRUE, FALSE);

    // Now FINALLY set the pixel data
    gimp_pixel_rgn_set_rect(&rgn,
                            image_data,
                            0, 0,
                            width, height);

    // We're done with the drawable
    gimp_drawable_flush(drawable);
    gimp_drawable_detach(drawable);

    // Free the image data
    free((void *)image_data);

    // Add the layer to the image
    gimp_image_add_layer(new_image_id, new_layer_id, 0);

    // Set the filename
    gimp_image_set_filename(new_image_id, filename);

    return new_image_id;
}

// Writes the file to disk
int write_webp(const gchar * filename, gint drawable_id, float quality)
{
    GimpDrawable * drawable;
    gint bpp;
    GimpPixelRgn rgn;
    long int data_size;
    void * image_data;
    size_t output_size;
    uint8_t * raw_data;
    FILE * file;

    // Get the drawable
    drawable = gimp_drawable_get(drawable_id);

    // Get the BPP
    bpp = gimp_drawable_bpp(drawable_id);

    // Get a pixel region from the layer
    gimp_pixel_rgn_init(&rgn,
                        drawable,
                        0, 0,
                        drawable->width,
                        drawable->height,
                        FALSE, FALSE);

    // Determine the size of the array of image data to get
    // and allocate it.
    data_size = drawable->width * drawable->height * bpp;
    image_data = malloc(data_size);

    // Get the image data
    gimp_pixel_rgn_get_rect(&rgn,
                            (guchar *)image_data,
                            0, 0,
                            drawable->width,
                            drawable->height);

    // We have the image data, now encode it.
    output_size = WebPEncodeRGB((const uint8_t *)image_data,
                                drawable->width,
                                drawable->height,
                                drawable->width * 3,
                                quality,
                                &raw_data);

    // Free the image data
    free(image_data);

    // Detach the drawable
    gimp_drawable_detach(drawable);

    // Make sure that the write was successful
    if(output_size == FALSE)
    {
        free(raw_data);
        return 0;
    }

    // Open the file
    file = fopen(filename, "wb");
    if(!file)
    {
        free(raw_data);
        return 0;
    }

    // Write the data and be done with it.
    fwrite(raw_data, output_size, 1, file);
    free(raw_data);
    fclose(file);

    return 1;
}

// Response structure
struct webp_data {
    int       * response;
    GtkObject * quality_scale;
    float     * quality;
};

void on_response(GtkDialog * dialog,
                 gint response_id,
                 gpointer user_data)
{
    // Basically all we want to do is grab the value
    // of the slider and store it in user_data.
    struct webp_data * data   = user_data;
    GtkHScale * hscale = GIMP_SCALE_ENTRY_SCALE(data->quality_scale);
    gdouble returned_quality;

    // Get the value
    returned_quality = gtk_range_get_value(GTK_RANGE(hscale));
    *(data->quality) = returned_quality;

    // Quit the loop
    gtk_main_quit();

    if(response_id == GTK_RESPONSE_OK)
        *(data->response) = 1;
}

// Displays the save options
int save_dialog(float * quality)
{
    int response = 0;
    struct webp_data data;
    GtkWidget * dialog;
    GtkWidget * vbox;
    GtkWidget * label;
    GtkWidget * table;
    GtkObject * scale;

    // Create the dialog
    dialog = gimp_dialog_new("Save as WebP",
                             BINARY_NAME,
                             NULL, 0,
                             NULL,
                             SAVE_PROCEDURE,
                             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                             GTK_STOCK_SAVE,   GTK_RESPONSE_OK,
                             NULL);

    // Create the VBox
    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       vbox, TRUE, TRUE, 2);
    gtk_widget_show(vbox);

    // Create the label
    label = gtk_label_new("The options below allow you to customize\nthe WebP image that is created.");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 2);
    gtk_widget_show(label);

    // Create the table
    table = gtk_table_new(1, 3, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 6);
    gtk_widget_show(table);

    // Create the scale
    scale = gimp_scale_entry_new(GTK_TABLE(table), 0, 0,
                                 "Quality:",
                                 150, 0,
                                 90.0f, 0.0f, 100.0f, 1.0f, 10.0f,
                                 0, TRUE, 0.0f, 0.0f,
                                 "Quality for encoding the image",
                                 NULL);

    // Connect to the response signal
    data.response      = &response;
    data.quality_scale = scale;
    data.quality       = quality;

    g_signal_connect(dialog, "response", G_CALLBACK(on_response),   &data);
    g_signal_connect(dialog, "destroy",  G_CALLBACK(gtk_main_quit), NULL);

    // Now show and run the dialog.
    gtk_widget_show(dialog);
    gtk_main();

    return response;
}
