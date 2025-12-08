#include <gtk/gtk.h>
#include <ImageStreamIO/ImageStreamIO.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Application state
typedef struct {
    IMAGE *image;
    GtkWidget *picture;
    char *image_name;
    guint timeout_id;
    double min_val;
    double max_val;
    gboolean fixed_min;
    gboolean fixed_max;
} ViewerApp;

// Command line option variables
static double opt_min = 0;
static double opt_max = 0;
static gboolean has_min = FALSE;
static gboolean has_max = FALSE;

// Custom callback to flag if options were set
static gboolean
parse_min_cb (const gchar *option_name,
              const gchar *value,
              gpointer     data,
              GError     **error)
{
    opt_min = g_ascii_strtod (value, NULL);
    has_min = TRUE;
    return TRUE;
}

static gboolean
parse_max_cb (const gchar *option_name,
              const gchar *value,
              gpointer     data,
              GError     **error)
{
    opt_max = g_ascii_strtod (value, NULL);
    has_max = TRUE;
    return TRUE;
}

static GOptionEntry entries[] =
{
  { "min", 'm', G_OPTION_FLAG_NONE, G_OPTION_ARG_CALLBACK, parse_min_cb, "Minimum value for scaling", "VAL" },
  { "max", 'M', G_OPTION_FLAG_NONE, G_OPTION_ARG_CALLBACK, parse_max_cb, "Maximum value for scaling", "VAL" },
  { NULL }
};

static void
draw_image (ViewerApp *app)
{
    if (!app->image || !app->image->array.raw) return;

    void *raw_data = NULL;

    // Fallback logic for reading buffer
    if (app->image->md->imagetype & CIRCULAR_BUFFER) {
        if (app->image->md->naxis == 3) {
            // cnt1 is the last slice written.
            // We use modulo just in case it is a monotonic counter,
            // though usually it should be the index itself.
            uint64_t slice_index = app->image->md->cnt1 % app->image->md->size[2];

            size_t element_size = ImageStreamIO_typesize(app->image->md->datatype);
            size_t frame_size = app->image->md->size[0] * app->image->md->size[1] * element_size;
            raw_data = (char*)app->image->array.raw + (slice_index * frame_size);
        } else {
             raw_data = app->image->array.raw;
        }
    } else {
        raw_data = app->image->array.raw;
    }

    if (!raw_data) return;

    int width = app->image->md->size[0];
    int height = app->image->md->size[1];
    uint8_t datatype = app->image->md->datatype;

    // Create a buffer for RGBA data
    int stride = width * 4;
    guchar *pixels = g_malloc (stride * height);

    double min_val = 1e30;
    double max_val = -1e30;

    // Optimization: if fixed_min and fixed_max are true, skip scan.
    gboolean need_scan = (!app->fixed_min || !app->fixed_max);

    if (need_scan) {
        for (int i = 0; i < width * height; i++) {
            double val = 0;
            if (datatype == _DATATYPE_FLOAT) {
                val = ((float*)raw_data)[i];
            } else if (datatype == _DATATYPE_DOUBLE) {
                val = ((double*)raw_data)[i];
            } else if (datatype == _DATATYPE_UINT8) {
                val = ((uint8_t*)raw_data)[i];
            } else if (datatype == _DATATYPE_INT16) {
                 val = ((int16_t*)raw_data)[i];
            } else if (datatype == _DATATYPE_UINT16) {
                 val = ((uint16_t*)raw_data)[i];
            } else if (datatype == _DATATYPE_INT32) {
                 val = ((int32_t*)raw_data)[i];
            } else if (datatype == _DATATYPE_UINT32) {
                 val = ((uint32_t*)raw_data)[i];
            }

            if (val < min_val) min_val = val;
            if (val > max_val) max_val = val;
        }
    }

    if (app->fixed_min) min_val = app->min_val;
    if (app->fixed_max) max_val = app->max_val;

    if (max_val == min_val) max_val = min_val + 1.0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
             double val = 0;
            if (datatype == _DATATYPE_FLOAT) {
                val = ((float*)raw_data)[idx];
            } else if (datatype == _DATATYPE_DOUBLE) {
                val = ((double*)raw_data)[idx];
            } else if (datatype == _DATATYPE_UINT8) {
                val = ((uint8_t*)raw_data)[idx];
            } else if (datatype == _DATATYPE_INT16) {
                val = ((int16_t*)raw_data)[idx];
            } else if (datatype == _DATATYPE_UINT16) {
                val = ((uint16_t*)raw_data)[idx];
            } else if (datatype == _DATATYPE_INT32) {
                val = ((int32_t*)raw_data)[idx];
            } else if (datatype == _DATATYPE_UINT32) {
                val = ((uint32_t*)raw_data)[idx];
            }

            // Clip values
            if (val < min_val) val = min_val;
            if (val > max_val) val = max_val;

            uint8_t pixel_val = (uint8_t)((val - min_val) / (max_val - min_val) * 255.0);

            // RGBA
            int p_idx = (y * width + x) * 4;
            pixels[p_idx + 0] = pixel_val;
            pixels[p_idx + 1] = pixel_val;
            pixels[p_idx + 2] = pixel_val;
            pixels[p_idx + 3] = 255;
        }
    }

    GBytes *bytes = g_bytes_new_take (pixels, stride * height);
    GdkTexture *texture = gdk_memory_texture_new (width, height, GDK_MEMORY_R8G8B8A8, bytes, stride);
    g_bytes_unref (bytes);

    gtk_picture_set_paintable (GTK_PICTURE (app->picture), GDK_PAINTABLE (texture));
    g_object_unref (texture);
}

static gboolean
update_display (gpointer user_data)
{
    ViewerApp *app = (ViewerApp *)user_data;

    // Check if image is connected
    if (!app->image) {
        app->image = (IMAGE*) malloc(sizeof(IMAGE));
        if (ImageStreamIO_openIm(app->image, app->image_name) != IMAGESTREAMIO_SUCCESS) {
             free(app->image);
             app->image = NULL;
             return G_SOURCE_CONTINUE;
        }
        printf("Connected to stream: %s\n", app->image_name);
        printf("Size: %ld x %ld\n", (long)app->image->md->size[0], (long)app->image->md->size[1]);
    }

    // Check update counter
    static uint64_t last_cnt0 = 0;

    if (app->image->md->cnt0 != last_cnt0) {
        if (app->image->md->write) {
             return G_SOURCE_CONTINUE;
        }

        last_cnt0 = app->image->md->cnt0;
        draw_image(app);
    }

    return G_SOURCE_CONTINUE;
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
    ViewerApp *viewer = (ViewerApp *)user_data;
    GtkWidget *window;
    GtkWidget *scrolled_window;
    GtkWidget *box;

    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "ImageStreamIO Viewer");
    gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child (GTK_WINDOW (window), box);

    scrolled_window = gtk_scrolled_window_new ();
    gtk_widget_set_vexpand (scrolled_window, TRUE);
    gtk_box_append (GTK_BOX (box), scrolled_window);

    viewer->picture = gtk_picture_new ();
    gtk_picture_set_content_fit (GTK_PICTURE (viewer->picture), GTK_CONTENT_FIT_CONTAIN);
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled_window), viewer->picture);

    viewer->timeout_id = g_timeout_add (30, update_display, viewer);

    gtk_window_present (GTK_WINDOW (window));
}

int
main (int    argc,
      char **argv)
{
    GtkApplication *app;
    int status;
    ViewerApp viewer = {0};
    GError *error = NULL;
    GOptionContext *context;

    // Parse command line arguments
    context = g_option_context_new ("<stream_name> - ImageStreamIO Viewer");
    g_option_context_add_main_entries (context, entries, NULL);
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_print ("option parsing failed: %s\n", error->message);
        exit (1);
    }
    g_option_context_free (context);

    if (argc < 2) {
        printf("Usage: %s [options] <stream_name>\n", argv[0]);
        return 1;
    }

    viewer.image_name = argv[1];
    viewer.min_val = opt_min;
    viewer.max_val = opt_max;
    viewer.fixed_min = has_min;
    viewer.fixed_max = has_max;

    if (viewer.fixed_min) printf("Fixed min: %f\n", viewer.min_val);
    if (viewer.fixed_max) printf("Fixed max: %f\n", viewer.max_val);

    app = gtk_application_new ("org.milk.shmimview", G_APPLICATION_NON_UNIQUE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), &viewer);
    status = g_application_run (G_APPLICATION (app), 0, NULL);
    g_object_unref (app);

    if (viewer.image) {
        ImageStreamIO_closeIm(viewer.image);
        free(viewer.image);
    }

    return status;
}
