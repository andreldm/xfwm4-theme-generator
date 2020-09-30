#include <gtk/gtk.h>

static GdkRGBA background_active;
static GdkRGBA background_inactive;
static GdkRGBA title_active;
static GdkRGBA title_inactive;
static GtkWidget *minimize_button;
static GtkWidget *maximize_button;
static GtkWidget *close_button;
static GtkWidget *headerbar;
static GtkWidget *title;
static GtkWidget *dialog;

void execute_steps ();

gboolean resume ()
{
  execute_steps ();
  return FALSE;
}

void headerbar_screenshot (gboolean active)
{
  GtkAllocation allocation;
  cairo_surface_t *surface, *clipped_surface;
  cairo_t *cr;
  gchar *filename, *color;
  GdkPixbuf *pixbuf;
  const guint8 *pixels;

  gtk_widget_get_allocation (headerbar, &allocation);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, allocation.width, allocation.height);
  cr = cairo_create (surface);

  gtk_widget_draw (headerbar, cr);

  clipped_surface = cairo_surface_create_for_rectangle (surface, 8, 0, 2, allocation.height);
  filename = g_strdup_printf ("theme/title-1-%s.png", active ? "active" : "inactive");
  cairo_surface_write_to_png (clipped_surface, filename);
  g_free (filename);
  cairo_surface_destroy (clipped_surface);

  clipped_surface = cairo_surface_create_for_rectangle (surface, 0, 0, 8, allocation.height);
  filename = g_strdup_printf ("theme/top-left-%s.png", active ? "active" : "inactive");
  cairo_surface_write_to_png (clipped_surface, filename);
  g_free (filename);
  cairo_surface_destroy (clipped_surface);

  clipped_surface = cairo_surface_create_for_rectangle (surface, allocation.width - 8, 0, 8, allocation.height);
  filename = g_strdup_printf ("theme/top-right-%s.png", active ? "active" : "inactive");
  cairo_surface_write_to_png (clipped_surface, filename);
  g_free (filename);
  cairo_surface_destroy (clipped_surface);

  pixbuf = gdk_pixbuf_get_from_surface (surface, 8, 8, 1, 1);
  pixels = gdk_pixbuf_read_pixels (pixbuf);
  color = g_strdup_printf ("rgba(%d,%d,%d,%d)", pixels[0], pixels[1], pixels[2], pixels[3]);
  gdk_rgba_parse (active ? &background_active : &background_inactive, color);
  g_free (color);
  g_object_unref (G_OBJECT (pixbuf));

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
}

void button_screenshot (GtkWidget *widget, const gchar *filename)
{
  GtkAllocation allocation;
  cairo_surface_t *surface;
  cairo_t *cr;

  gtk_widget_get_allocation (widget, &allocation);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, allocation.width, allocation.height);
  cr = cairo_create (surface);

  gdk_cairo_set_source_rgba (cr, g_str_has_suffix (filename, "inactive.png") ?
                                 &background_inactive : &background_active);
  cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
  cairo_fill (cr);
  gtk_widget_draw (widget, cr);
  cairo_surface_write_to_png (surface, filename);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
}

void buttons_screenshots (const gchar *suffix)
{
  gchar *filename;

  filename = g_strdup_printf ("theme/hide-%s.png", suffix);
  button_screenshot (minimize_button, filename);
  g_free (filename);

  filename = g_strdup_printf ("theme/maximize-%s.png", suffix);
  button_screenshot (maximize_button, filename);
  g_free (filename);

  filename = g_strdup_printf ("theme/close-%s.png", suffix);
  button_screenshot (close_button, filename);
  g_free (filename);
}

void prepare_buttons (GtkStateFlags state)
{
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (minimize_button);
  gtk_style_context_set_state (context, state);
  context = gtk_widget_get_style_context (maximize_button);
  gtk_style_context_set_state (context, state);
  context = gtk_widget_get_style_context (close_button);
  gtk_style_context_set_state (context, state);
}

void prepare_headerbar_inactive ()
{
  dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_NONE, NULL);
  gtk_widget_show (dialog);
}

void get_title_color (GdkRGBA *rgba)
{
  GtkStyleContext *context = gtk_widget_get_style_context (title);
  gtk_style_context_get_color (context, GTK_STATE_FLAG_NORMAL, rgba);
}

void generate_themerc ()
{
  gchar *contents;

  contents = g_strdup_printf ("active_text_color=#%02X%02X%02X\n\
inactive_text_color=#%02X%02X%02X\n\
title_shadow_active=false\n\
title_shadow_inactive=false\n\
full_width_title=true\n\
title_vertical_offset_active=2\n\
title_vertical_offset_inactive=2\n\
button_offset=4\n\
button_spacing=6\n\
shadow_delta_height=2\n\
shadow_delta_width=0\n\
shadow_delta_x=0\n\
shadow_delta_y=-10\n\
shadow_opacity=50\n\
show_app_icon=true",
  (int)(title_active.red*255), (int)(title_active.green*255), (int)(title_active.blue*255),
  (int)(title_inactive.red*255), (int)(title_inactive.green*255), (int)(title_inactive.blue*255));

  g_file_set_contents ("theme/themerc", contents, -1, NULL);
  g_free (contents);
}

void generate_borders ()
{
  cairo_surface_t *surface, *clipped_surface;
  cairo_t *cr;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 24, 24);
  cr = cairo_create (surface);
  gdk_cairo_set_source_rgba (cr, &background_active);
  cairo_rectangle (cr, 0, 0, 24, 24);
  cairo_fill (cr);

  cairo_surface_write_to_png (surface, "theme/menu-active.png");
  cairo_surface_write_to_png (surface, "theme/menu-inactive.png");

  clipped_surface = cairo_surface_create_for_rectangle (surface, 0, 0, 3, 24);
  cairo_surface_write_to_png (clipped_surface, "theme/right-active.png");
  cairo_surface_write_to_png (clipped_surface, "theme/right-inactive.png");
  cairo_surface_write_to_png (clipped_surface, "theme/left-active.png");
  cairo_surface_write_to_png (clipped_surface, "theme/left-inactive.png");
  cairo_surface_destroy (clipped_surface);

  clipped_surface = cairo_surface_create_for_rectangle (surface, 0, 0, 24, 3);
  cairo_surface_write_to_png (clipped_surface, "theme/bottom-active.png");
  cairo_surface_write_to_png (clipped_surface, "theme/bottom-inactive.png");
  cairo_surface_destroy (clipped_surface);

  cairo_set_source_rgba (cr, 0, 0, 0, 1);
  cairo_rectangle (cr, 3, 0, 21, 21);
  cairo_set_operator(cr,CAIRO_OPERATOR_CLEAR);
  cairo_fill (cr);
  cairo_surface_write_to_png (surface, "theme/bottom-left-active.png");
  cairo_surface_write_to_png (surface, "theme/bottom-left-inactive.png");

  gdk_cairo_set_source_rgba (cr, &background_active);
  cairo_rectangle (cr, 0, 0, 24, 24);
  cairo_set_operator(cr,CAIRO_OPERATOR_OVER);
  cairo_fill (cr);
  cairo_set_source_rgba (cr, 0, 0, 0, 0);
  cairo_rectangle (cr, 0, 0, 21, 21);
  cairo_set_operator(cr,CAIRO_OPERATOR_CLEAR);
  cairo_fill (cr);
  cairo_surface_write_to_png (surface, "theme/bottom-right-active.png");
  cairo_surface_write_to_png (surface, "theme/bottom-right-inactive.png");

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
}

#define STEP(num, delay_after, action) \
  case num: action; g_timeout_add (delay_after, resume, NULL); break;

void execute_steps ()
{
  static gint step = 0;

  switch(step++) {
    STEP (0, 0, g_mkdir_with_parents ("theme", 0755));
    STEP (1, 0, headerbar_screenshot (TRUE));
    STEP (2, 250, prepare_headerbar_inactive ());
    STEP (3, 0, headerbar_screenshot (FALSE));
    STEP (4, 250, prepare_buttons (GTK_STATE_FLAG_BACKDROP));
    STEP (5, 0, buttons_screenshots ("inactive"));
    STEP (6, 0, get_title_color (&title_inactive));
    STEP (7, 250, gtk_widget_destroy (dialog));
    STEP (8, 0, get_title_color (&title_active));
    STEP (9, 250, prepare_buttons (GTK_STATE_FLAG_PRELIGHT));
    STEP (10, 0, buttons_screenshots ("prelight"));
    STEP (11, 250, prepare_buttons (GTK_STATE_FLAG_ACTIVE));
    STEP (12, 0, buttons_screenshots ("pressed"));
    STEP (13, 250, prepare_buttons (GTK_STATE_FLAG_NORMAL));
    STEP (14, 0, buttons_screenshots ("active"));
    STEP (15, 0, generate_themerc ());
    STEP (16, 0, generate_borders ());
    default: gtk_main_quit ();
  }
}

void search_widgets (GtkWidget *widget, gpointer data)
{
  GtkStyleContext *context;

  if (GTK_IS_BUTTON (widget)) {
    context = gtk_widget_get_style_context (widget);

    if (gtk_style_context_has_class (context, "minimize"))
      minimize_button = widget;
    else if (gtk_style_context_has_class (context, "maximize"))
      maximize_button = widget;
    else if (gtk_style_context_has_class (context, "close"))
      close_button = widget;
  }

  if (GTK_IS_LABEL (widget)) {
    context = gtk_widget_get_style_context (widget);

    if (gtk_style_context_has_class (context, "title"))
      title = widget;
  }

  if (GTK_IS_HEADER_BAR (widget)) {
    headerbar = widget;
  }

  if (GTK_IS_CONTAINER (widget)) {
    gtk_container_forall (GTK_CONTAINER (widget), search_widgets, data);
  }
}

gboolean callback (gpointer data)
{
  gtk_container_forall (GTK_CONTAINER (data), search_widgets, gtk_widget_get_window (GTK_WIDGET (data)));
  execute_steps ();
  return FALSE;
}

int main (int argc, char *argv[])
{
  static GtkWidget *window;

  putenv("GTK_CSD=1");
  // putenv("GDK_SCALE=2");
  // putenv("GTK_THEME=Adwaita");

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Sample");
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW (window), 320, 240);
  gtk_widget_show_all (window);

  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);
  g_timeout_add (250, callback, window);

  gtk_main ();

  return 0;
}
