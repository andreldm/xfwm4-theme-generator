#include <gtk/gtk.h>

static GdkRGBA background_color;
static GdkRGBA title_active;
static GdkRGBA title_inactive;
static GdkPixbuf *active_pixbuf;
static GdkPixbuf *inactive_pixbuf;
static GtkStyleContext *window_context, *decoration_context, *headerbar_context, *title_context, *button_context, *image_context;
static gint scale, headerbar_height, button_width, button_height;

static GtkStyleContext * create_style_context (GtkStyleContext *, const gchar *, const gchar *, ...);
static void buttons_screenshots (const gchar *, GtkStateFlags);
static void button_screenshot (const gchar *, const gchar *);
static void headerbar_screenshot (gboolean);
static void generate_themerc ();
static void generate_borders ();
static void get_title_color ();
static void get_headerbar_height();
static void get_button_dimentions();

int main (int argc, char *argv[])
{
  gtk_init (&argc, &argv);

  scale = 1; // TODO read from argument or system wide settings
  window_context = create_style_context (NULL, "window", GTK_STYLE_CLASS_BACKGROUND, "csd", "metacity", NULL); // "solid-csd" is also accepted
  decoration_context = create_style_context (window_context, "decoration", NULL);
  headerbar_context = create_style_context (window_context, "headerbar", GTK_STYLE_CLASS_TITLEBAR, GTK_STYLE_CLASS_HORIZONTAL, "default-decoration", NULL);
  title_context = create_style_context (headerbar_context, "label", GTK_STYLE_CLASS_TITLE, NULL);
  button_context = create_style_context (headerbar_context, "button", "titlebutton", NULL);
  image_context = create_style_context (button_context, "image", NULL);

  get_headerbar_height ();
  get_button_dimentions ();

  g_mkdir_with_parents ("theme", 0755);
  headerbar_screenshot (TRUE);
  buttons_screenshots ("prelight", GTK_STATE_FLAG_PRELIGHT);
  buttons_screenshots ("pressed", GTK_STATE_FLAG_ACTIVE);
  buttons_screenshots ("active", GTK_STATE_FLAG_NORMAL);
  headerbar_screenshot (FALSE);
  buttons_screenshots ("inactive", GTK_STATE_FLAG_BACKDROP);
  get_title_color ();
  generate_themerc ();
  generate_borders ();

  return 0;
}

static GtkStyleContext * create_style_context (GtkStyleContext *parent, const gchar *object_name, const gchar *first_class, ...)
{
  GtkWidgetPath *path;
  GtkStyleContext *context;
  const gchar *name;
  va_list ap;

  path = parent ?
    gtk_widget_path_copy (gtk_style_context_get_path (parent)) :
    gtk_widget_path_new ();

  gtk_widget_path_append_type (path, G_TYPE_NONE);
  gtk_widget_path_iter_set_object_name (path, -1, object_name);

  va_start (ap, first_class);
  for (name = first_class; name; name = va_arg (ap, const gchar *))
    gtk_widget_path_iter_add_class (path, -1, name);
  va_end (ap);

  context = gtk_style_context_new ();
  gtk_style_context_set_path (context, path);
  gtk_style_context_set_parent (context, parent);
  gtk_style_context_set_scale (context, scale);
  gtk_widget_path_unref (path);

  return context;
}

static void buttons_screenshots (const gchar *suffix, GtkStateFlags state)
{
  gchar *filename;
  static GtkStateFlags original_state = 0;

  if (!original_state)
    original_state = gtk_style_context_get_state (button_context);
  gtk_style_context_set_state (button_context, original_state | state);

  filename = g_strdup_printf ("theme/hide-%s.png", suffix);
  button_screenshot ("minimize", filename);
  g_free (filename);

  filename = g_strdup_printf ("theme/maximize-%s.png", suffix);
  button_screenshot ("maximize", filename);
  g_free (filename);

  filename = g_strdup_printf ("theme/close-%s.png", suffix);
  button_screenshot ("close", filename);
  g_free (filename);
}

static void button_screenshot (const gchar *style_class, const gchar *filename)
{
  gint x, y;
  cairo_surface_t *surface;
  cairo_t *cr;
  const gchar *icon_name;
  const gint icon_size = 16;
  GError *error = NULL;

  x = y = 0;
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, button_width, button_height);
  cr = cairo_create (surface);

  /* Draw background */
  gtk_render_background (headerbar_context, cr, x - 16, y - 4, button_width + 32, button_height + 8);

  gtk_style_context_add_class (button_context, style_class);
  gtk_render_background (button_context, cr, x, y, button_width, button_height);
  gtk_render_frame (button_context, cr, x, y, button_width, button_height);

  /* Draw the symbolic icon, properly colorized for the current style context */
  if (g_strcmp0 (style_class, "minimize") == 0)
    icon_name = "window-minimize-symbolic";
  else if (g_strcmp0 (style_class, "maximize") == 0)
    icon_name = "window-maximize-symbolic";
  else
    icon_name = "window-close-symbolic";

  GtkIconInfo *icon_info = gtk_icon_theme_lookup_icon (
      gtk_icon_theme_get_default (), icon_name, icon_size, GTK_ICON_LOOKUP_FORCE_SYMBOLIC);

  if (icon_info) {
    GdkPixbuf *pixbuf = gtk_icon_info_load_symbolic_for_context (icon_info, button_context, NULL, &error);
    g_object_unref (icon_info);
    if (pixbuf) {
      gdk_cairo_set_source_pixbuf (cr, pixbuf, (button_width - icon_size) / 2, (button_height - icon_size) / 2);
      cairo_paint (cr);
      g_object_unref (pixbuf);
    } else {
      g_warning ("Failed to load icon %s: %s", icon_name, error ? error->message : "(no icon info)");
      g_clear_error (&error);
    }
  }

  cairo_surface_write_to_png (surface, filename);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  gtk_style_context_remove_class (button_context, style_class);
}

static void headerbar_screenshot (gboolean active)
{
  gint width, height;
  cairo_surface_t *surface, *clipped_surface;
  cairo_t *cr;
  gchar *filename, *color;
  GdkPixbuf *pixbuf;
  const guint8 *pixels;
  static GtkStateFlags original_state = 0;

  width = 300;
  height = headerbar_height;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_create (surface);

  if (!original_state)
    original_state = gtk_style_context_get_state (headerbar_context);
  gtk_style_context_set_state (headerbar_context, original_state | (active ? GTK_STATE_FLAG_NORMAL : GTK_STATE_FLAG_BACKDROP));

  gtk_render_background (headerbar_context, cr, 0, 0, width, height);
  gtk_render_frame (headerbar_context, cr, 0, 0, width, height);

  clipped_surface = cairo_surface_create_for_rectangle (surface, 96, 0, 2, height);
  filename = g_strdup_printf ("theme/title-1-%s.png", active ? "active" : "inactive");
  cairo_surface_write_to_png (clipped_surface, filename);
  g_free (filename);
  cairo_surface_destroy (clipped_surface);

  clipped_surface = cairo_surface_create_for_rectangle (surface, 0, 0, 8, height);
  filename = g_strdup_printf ("theme/top-left-%s.png", active ? "active" : "inactive");
  cairo_surface_write_to_png (clipped_surface, filename);
  g_free (filename);
  cairo_surface_destroy (clipped_surface);

  clipped_surface = cairo_surface_create_for_rectangle (surface, width - 8, 0, 8, height);
  filename = g_strdup_printf ("theme/top-right-%s.png", active ? "active" : "inactive");
  cairo_surface_write_to_png (clipped_surface, filename);
  g_free (filename);
  cairo_surface_destroy (clipped_surface);

  if (active) {
    active_pixbuf = gdk_pixbuf_get_from_surface (surface, 96, 4, 1, height - 8);

    pixbuf = gdk_pixbuf_get_from_surface (surface, 96, 8, 1, 1);
    pixels = gdk_pixbuf_read_pixels (pixbuf);
    color = g_strdup_printf ("rgba(%d,%d,%d,%d)", pixels[0], pixels[1], pixels[2], pixels[3]);
    gdk_rgba_parse (&background_color, color);
    g_free (color);
    g_object_unref (G_OBJECT (pixbuf));
  }
  else {
    inactive_pixbuf = gdk_pixbuf_get_from_surface (surface, 96, 4, 1, height - 8);
  }

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
}

static void generate_themerc ()
{
  gchar *contents, *theme_name;
  GtkSettings *settings;

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

  settings = gtk_settings_get_default ();
  g_object_get (settings, "gtk-theme-name", &theme_name, NULL);
  g_file_set_contents ("theme/.name", theme_name, -1, NULL);
  g_free (theme_name);
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
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_fill (cr);
  cairo_surface_write_to_png (surface, "theme/bottom-left-active.png");
  cairo_surface_write_to_png (surface, "theme/bottom-left-inactive.png");

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  gdk_cairo_set_source_rgba (cr, &background_color);
  cairo_rectangle (cr, 0, 0, 24, 24);
  cairo_fill (cr);
  cairo_set_source_rgba (cr, 0, 0, 0, 0);
  cairo_rectangle (cr, 0, 0, 21, 21);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_fill (cr);
  cairo_surface_write_to_png (surface, "theme/bottom-right-active.png");
  cairo_surface_write_to_png (surface, "theme/bottom-right-inactive.png");

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

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

static void get_title_color ()
{
  gtk_style_context_get_color (title_context, GTK_STATE_FLAG_NORMAL, &title_active);
  gtk_style_context_get_color (title_context, GTK_STATE_FLAG_INSENSITIVE, &title_inactive);
}

static void get_headerbar_height ()
{
  GtkAllocation allocation;
  GtkWidget *offscreen_window, *headerbar;
  GtkStyleContext *context;
  GtkCssProvider *css_provider;

  offscreen_window = gtk_offscreen_window_new ();
  headerbar = gtk_header_bar_new ();
  context = gtk_widget_get_style_context (headerbar);
  gtk_style_context_add_class (context, "titlebar");
  /* The following class makes the titlebar taller, which is good for Adwaita but bad for Greybird */
  /* gtk_style_context_add_class (context, "default-decoration"); */

  css_provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (css_provider, ".titlebar { min-height: 1px; }", -1, NULL);
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (css_provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  gtk_container_add (GTK_CONTAINER (offscreen_window), headerbar);
  gtk_widget_show_all (offscreen_window);
  gtk_widget_get_allocation (headerbar, &allocation);
  gtk_widget_destroy (offscreen_window);

  headerbar_height = allocation.height;
}

static void get_button_dimentions ()
{
  GtkAllocation allocation;
  GtkWidget *offscreen_window;
  GtkWidget *button;
  GtkStyleContext *context;

  offscreen_window = gtk_offscreen_window_new ();
  button = gtk_button_new ();
  context = gtk_widget_get_style_context (button);
  gtk_style_context_add_class (context, "titlebutton");
  gtk_style_context_set_parent (context, headerbar_context);

  gtk_container_add (GTK_CONTAINER (offscreen_window), button);
  gtk_widget_show_all (offscreen_window);
  gtk_widget_get_allocation (button, &allocation);
  gtk_widget_destroy (offscreen_window);

  button_width = allocation.width;
  button_height = allocation.height;
}
