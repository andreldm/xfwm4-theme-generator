#include <gtk/gtk.h>

static GdkRGBA background_color;
static GdkRGBA title_active;
static GdkRGBA title_inactive;
static GdkPixbuf *active_pixbuf;
static GdkPixbuf *inactive_pixbuf;
static GtkWidget *minimize_button;
static GtkWidget *maximize_button;
static GtkWidget *close_button;
static GtkWidget *headerbar;
static GtkWidget *title;
static GtkWidget *dialog;

static gboolean init (gpointer data);
static void search_widgets (GtkWidget *widget, gpointer data);
static void execute_steps ();
static void prepare_buttons (GtkStateFlags);
static void buttons_screenshots (const gchar *, gboolean);
static void button_screenshot (GtkWidget *, const gchar *, gboolean);
static void prepare_headerbar_inactive ();
static void headerbar_screenshot (gboolean);
static void generate_themerc ();
static void generate_borders ();
static void get_title_color (GdkRGBA *);
static gboolean resume ();

int main (int argc, char *argv[])
{
  GtkWidget *window, *label;

  putenv("GTK_CSD=1");
  // putenv("GDK_SCALE=2");
  // putenv("GTK_THEME=Adwaita");

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "");
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW (window), 320, 240);
  gtk_widget_show_all (window);

  label = gtk_label_new ("Please wait while the theme is generated.");
  gtk_container_add (GTK_CONTAINER (window), label);
  gtk_widget_show (label);

  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);
  g_timeout_add (250, init, window);

  gtk_main ();

  return 0;
}

static gboolean init (gpointer data)
{
  gtk_container_forall (GTK_CONTAINER (data), search_widgets, gtk_widget_get_window (GTK_WIDGET (data)));
  execute_steps ();
  return FALSE;
}

static void search_widgets (GtkWidget *widget, gpointer data)
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
#define STEP(num, delay_after, action) \
  case num: action; g_timeout_add (delay_after, resume, NULL); break;

static void execute_steps ()
{
  static gint step = 0;

  switch(step++) {
    STEP (0, 0, g_mkdir_with_parents ("theme", 0755));
    STEP (1, 0, headerbar_screenshot (TRUE));
    STEP (2, 250, prepare_headerbar_inactive ());
    STEP (3, 0, headerbar_screenshot (FALSE));
    STEP (4, 250, prepare_buttons (GTK_STATE_FLAG_BACKDROP));
    STEP (5, 0, buttons_screenshots ("inactive", FALSE));
    STEP (6, 0, get_title_color (&title_inactive));
    STEP (7, 250, gtk_widget_destroy (dialog));
    STEP (8, 0, get_title_color (&title_active));
    STEP (9, 250, prepare_buttons (GTK_STATE_FLAG_PRELIGHT));
    STEP (10, 0, buttons_screenshots ("prelight", TRUE));
    STEP (11, 250, prepare_buttons (GTK_STATE_FLAG_ACTIVE));
    STEP (12, 0, buttons_screenshots ("pressed", TRUE));
    STEP (13, 250, prepare_buttons (GTK_STATE_FLAG_NORMAL));
    STEP (14, 0, buttons_screenshots ("active", TRUE));
    STEP (15, 0, generate_themerc ());
    STEP (16, 0, generate_borders ());
    default:
      g_object_unref (G_OBJECT (active_pixbuf));
      g_object_unref (G_OBJECT (inactive_pixbuf));
      gtk_main_quit ();
  }
}

#undef STEP

static void prepare_buttons (GtkStateFlags state)
{
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (minimize_button);
  gtk_style_context_set_state (context, state);
  context = gtk_widget_get_style_context (maximize_button);
  gtk_style_context_set_state (context, state);
  context = gtk_widget_get_style_context (close_button);
  gtk_style_context_set_state (context, state);
}

static void buttons_screenshots (const gchar *suffix, gboolean active)
{
  gchar *filename;

  filename = g_strdup_printf ("theme/hide-%s.png", suffix);
  button_screenshot (minimize_button, filename, active);
  g_free (filename);

  filename = g_strdup_printf ("theme/maximize-%s.png", suffix);
  button_screenshot (maximize_button, filename, active);
  g_free (filename);

  filename = g_strdup_printf ("theme/close-%s.png", suffix);
  button_screenshot (close_button, filename, active);
  g_free (filename);
}

static void button_screenshot (GtkWidget *widget, const gchar *filename, gboolean active)
{
  GtkAllocation allocation;
  cairo_surface_t *surface, *pattern_surface;
  cairo_pattern_t *pattern;
  cairo_t *cr;

  gtk_widget_get_allocation (widget, &allocation);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, allocation.width, allocation.height);
  cr = cairo_create (surface);

  /* Draw background pattern */
  pattern_surface = gdk_cairo_surface_create_from_pixbuf (active ? active_pixbuf : inactive_pixbuf, 0, NULL);
  cairo_set_source_surface (cr, pattern_surface, 0, 0);
  pattern = cairo_get_source (cr);
  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
  cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
  cairo_fill (cr);
  cairo_surface_destroy (pattern_surface);

  gtk_widget_draw (widget, cr);
  cairo_surface_write_to_png (surface, filename);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
}

static void prepare_headerbar_inactive ()
{
  dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_OTHER, GTK_BUTTONS_NONE, NULL);
  gtk_widget_show (dialog);
}

static void headerbar_screenshot (gboolean active)
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

  clipped_surface = cairo_surface_create_for_rectangle (surface, 96, 0, 2, allocation.height);
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

  if (active) {
    active_pixbuf = gdk_pixbuf_get_from_surface (surface, 96, 4, 1, allocation.height - 8);

    pixbuf = gdk_pixbuf_get_from_surface (surface, 96, 8, 1, 1);
    pixels = gdk_pixbuf_read_pixels (pixbuf);
    color = g_strdup_printf ("rgba(%d,%d,%d,%d)", pixels[0], pixels[1], pixels[2], pixels[3]);
    gdk_rgba_parse (&background_color, color);
    g_free (color);
    g_object_unref (G_OBJECT (pixbuf));
  }
  else {
    inactive_pixbuf = gdk_pixbuf_get_from_surface (surface, 96, 4, 1, allocation.height - 8);
  }

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
}

static void generate_themerc ()
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
maximized_offset=4\n\
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

static void generate_borders ()
{
  cairo_surface_t *surface, *clipped_surface, *pattern_surface;
  cairo_pattern_t *pattern;
  cairo_t *cr;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 24, 24);
  cr = cairo_create (surface);
  gdk_cairo_set_source_rgba (cr, &background_color);
  cairo_rectangle (cr, 0, 0, 24, 24);
  cairo_fill (cr);

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

  cairo_set_operator(cr,CAIRO_OPERATOR_OVER);

  gdk_cairo_set_source_rgba (cr, &background_color);
  cairo_rectangle (cr, 0, 0, 24, 24);
  cairo_fill (cr);
  cairo_set_source_rgba (cr, 0, 0, 0, 0);
  cairo_rectangle (cr, 0, 0, 21, 21);
  cairo_set_operator(cr,CAIRO_OPERATOR_CLEAR);
  cairo_fill (cr);
  cairo_surface_write_to_png (surface, "theme/bottom-right-active.png");
  cairo_surface_write_to_png (surface, "theme/bottom-right-inactive.png");

  cairo_set_operator(cr,CAIRO_OPERATOR_OVER);

  /* Draw menu active */
  pattern_surface = gdk_cairo_surface_create_from_pixbuf (active_pixbuf, 0, NULL);
  cairo_set_source_surface (cr, pattern_surface, 0, 0);
  pattern = cairo_get_source (cr);
  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
  cairo_rectangle (cr, 0, 0, 24, 24);
  cairo_fill (cr);
  cairo_surface_destroy (pattern_surface);
  cairo_surface_write_to_png (surface, "theme/menu-active.png");

  /* Draw menu inactive */
  pattern_surface = gdk_cairo_surface_create_from_pixbuf (inactive_pixbuf, 0, NULL);
  cairo_set_source_surface (cr, pattern_surface, 0, 0);
  pattern = cairo_get_source (cr);
  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
  cairo_rectangle (cr, 0, 0, 24, 24);
  cairo_fill (cr);
  cairo_surface_destroy (pattern_surface);
  cairo_surface_write_to_png (surface, "theme/menu-inactive.png");

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
}

static void get_title_color (GdkRGBA *rgba)
{
  GtkStyleContext *context = gtk_widget_get_style_context (title);
  gtk_style_context_get_color (context, GTK_STATE_FLAG_NORMAL, rgba);
}

static gboolean resume ()
{
  execute_steps ();
  return FALSE;
}
