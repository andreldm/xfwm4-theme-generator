#include <gtk/gtk.h>

static GdkRGBA background_color;
static GdkRGBA title_active;
static GdkRGBA title_inactive;
static GdkPixbuf *active_pixbuf;
static GdkPixbuf *inactive_pixbuf;
static GtkStyleContext *window_context, *headerbar_context, *title_context, *button_context;
static gint scale, headerbar_height, button_width, button_height;

static GtkStyleContext *create_style_context (GtkStyleContext *, const gchar *, const gchar *, ...);
static void screenshot_buttons (const gchar *, GtkStateFlags);
static void screenshot_button (const gchar *, const gchar *, const gchar *);
static void screenshot_headerbar (gboolean);
static void generate_themerc ();
static void generate_borders ();
static void determine_title_color ();
static void determine_headerbar_height ();

int main (int argc, char *argv[])
{
  gtk_init (&argc, &argv);

  scale = 1; // TODO read from argument or system wide settings
  window_context = create_style_context (NULL, "window", GTK_STYLE_CLASS_BACKGROUND, "csd", "metacity", NULL); // "solid-csd" is also accepted
  headerbar_context = create_style_context (window_context, "headerbar", GTK_STYLE_CLASS_TITLEBAR, GTK_STYLE_CLASS_HORIZONTAL, "default-decoration", NULL);
  title_context = create_style_context (headerbar_context, "label", GTK_STYLE_CLASS_TITLE, NULL);
  button_context = create_style_context (headerbar_context, "button", "titlebutton", NULL);

  g_mkdir_with_parents ("theme", 0755);

  determine_title_color ();
  determine_headerbar_height ();

  screenshot_headerbar (TRUE);
  screenshot_buttons ("prelight", GTK_STATE_FLAG_PRELIGHT);
  screenshot_buttons ("pressed", GTK_STATE_FLAG_ACTIVE);
  screenshot_buttons ("active", GTK_STATE_FLAG_NORMAL);
  screenshot_headerbar (FALSE);
  screenshot_buttons ("inactive", GTK_STATE_FLAG_BACKDROP);
  generate_borders ();
  generate_themerc ();

  return 0;
}

static GtkStyleContext *create_style_context (GtkStyleContext *parent, const gchar *object_name, const gchar *first_class, ...)
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

static void screenshot_buttons (const gchar *suffix, GtkStateFlags state)
{
  static const struct { const gchar *prefix; const gchar *class; const gchar *icon; } buttons[] = {
    { "hide",             "minimize", "window-minimize-symbolic" },
    { "maximize",         "maximize", "window-maximize-symbolic" },
    { "maximize-toggled", "maximize", "window-restore-symbolic"  },
    { "close",            "close",    "window-close-symbolic"    },
  };

  gtk_style_context_set_state (button_context, state);

  for (guint i = 0; i < G_N_ELEMENTS (buttons); i++) {
    gchar *filename = g_strdup_printf ("theme/%s-%s.png", buttons[i].prefix, suffix);
    screenshot_button (filename, buttons[i].class, buttons[i].icon);
    g_free (filename);
  }
}

static void screenshot_button (const gchar *filename, const gchar *style_class, const gchar *icon_name)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  const gint icon_size = 16;
  GError *error = NULL;
  GtkIconInfo *icon_info;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, button_width, button_height);
  cr = cairo_create (surface);

  gtk_render_background (headerbar_context, cr, -16, -4, button_width + 32, button_height + 8);

  gtk_style_context_add_class (button_context, style_class);
  gtk_render_background (button_context, cr, 0, 0, button_width, button_height);
  gtk_render_frame (button_context, cr, 0, 0, button_width, button_height);

  icon_info = gtk_icon_theme_lookup_icon (gtk_icon_theme_get_default (), icon_name, icon_size, GTK_ICON_LOOKUP_FORCE_SYMBOLIC);

  if (icon_info) {
    GdkPixbuf *pixbuf = gtk_icon_info_load_symbolic_for_context (icon_info, button_context, NULL, &error);
    g_object_unref (icon_info);
    if (pixbuf) {
      gdk_cairo_set_source_pixbuf (cr, pixbuf, (button_width - icon_size) / 2, (button_height - icon_size) / 2);
      cairo_paint (cr);
      g_object_unref (pixbuf);
    } else {
      g_warning ("Failed to load icon %s: %s", icon_name, error->message);
      g_clear_error (&error);
    }
  }

  cairo_surface_write_to_png (surface, filename);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  gtk_style_context_remove_class (button_context, style_class);
}

static void write_surface (cairo_surface_t *surface, gint x, gint y,
                           gint width, gint height, const gchar *filename)
{
  cairo_surface_t *clipped = cairo_surface_create_for_rectangle (surface, x, y, width, height);
  cairo_surface_write_to_png (clipped, filename);
  cairo_surface_destroy (clipped);
}

static void screenshot_headerbar (gboolean active)
{
  gint width, height;
  cairo_surface_t *surface;
  cairo_t *cr;
  gchar *filename, *color;
  GdkPixbuf *pixbuf;
  const guint8 *pixels;
  const gchar *state_name = active ? "active" : "inactive";

  width = 300;
  height = headerbar_height;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_create (surface);

  gtk_style_context_set_state (headerbar_context, active ? GTK_STATE_FLAG_NORMAL : GTK_STATE_FLAG_BACKDROP);

  gtk_render_background (headerbar_context, cr, 0, 0, width, height);
  gtk_render_frame (headerbar_context, cr, 0, 0, width, height);

  filename = g_strdup_printf ("theme/title-1-%s.png", state_name);
  write_surface (surface, 96, 0, 2, height, filename);
  g_free (filename);

  filename = g_strdup_printf ("theme/top-left-%s.png", state_name);
  write_surface (surface, 0, 0, 8, height, filename);
  g_free (filename);

  filename = g_strdup_printf ("theme/top-right-%s.png", state_name);
  write_surface (surface, width - 8, 0, 8, height, filename);
  g_free (filename);

  if (active) {
    active_pixbuf = gdk_pixbuf_get_from_surface (surface, 96, 4, 1, height - 8);

    pixbuf = gdk_pixbuf_get_from_surface (surface, 96, 8, 1, 1);
    pixels = gdk_pixbuf_read_pixels (pixbuf);
    color = g_strdup_printf ("rgba(%d,%d,%d,%d)", pixels[0], pixels[1], pixels[2], pixels[3]);
    gdk_rgba_parse (&background_color, color);
    g_free (color);
    g_object_unref (G_OBJECT (pixbuf));
  } else {
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

static void draw_menu (cairo_t *cr, cairo_surface_t *surface,
                       GdkPixbuf *pixbuf, const gchar *filename)
{
  cairo_surface_t *pattern_surface = gdk_cairo_surface_create_from_pixbuf (pixbuf, 0, NULL);
  cairo_set_source_surface (cr, pattern_surface, 0, 0);
  cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
  cairo_rectangle (cr, 0, 0, 24, 24);
  cairo_fill (cr);
  cairo_surface_destroy (pattern_surface);
  cairo_surface_write_to_png (surface, filename);
}

static void generate_borders ()
{
  cairo_surface_t *surface, *clipped_surface;
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
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_fill (cr);
  cairo_surface_write_to_png (surface, "theme/bottom-left-active.png");
  cairo_surface_write_to_png (surface, "theme/bottom-left-inactive.png");

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  gdk_cairo_set_source_rgba (cr, &background_color);
  cairo_rectangle (cr, 0, 0, 24, 24);
  cairo_fill (cr);
  cairo_set_source_rgba (cr, 0, 0, 0, 0);
  cairo_rectangle (cr, 0, 0, 21, 21);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_fill (cr);
  cairo_surface_write_to_png (surface, "theme/bottom-right-active.png");
  cairo_surface_write_to_png (surface, "theme/bottom-right-inactive.png");

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  draw_menu (cr, surface, active_pixbuf, "theme/menu-active.png");
  draw_menu (cr, surface, inactive_pixbuf, "theme/menu-inactive.png");

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
}

static void determine_title_color ()
{
  gtk_style_context_get_color (title_context, GTK_STATE_FLAG_NORMAL, &title_active);
  gtk_style_context_get_color (title_context, GTK_STATE_FLAG_INSENSITIVE, &title_inactive);
}

static void determine_headerbar_height ()
{
  GtkBorder hb_padding, hb_border, btn_margin, btn_border, btn_padding;
  gint hb_min_height, btn_min_height, btn_min_width;
  gint btn_content_h, btn_content_w, content;

  gtk_style_context_get_padding (headerbar_context, GTK_STATE_FLAG_NORMAL, &hb_padding);
  gtk_style_context_get_border (headerbar_context, GTK_STATE_FLAG_NORMAL, &hb_border);
  gtk_style_context_get (headerbar_context, GTK_STATE_FLAG_NORMAL,
                         "min-height", &hb_min_height, NULL);

  gtk_style_context_get_margin (button_context, GTK_STATE_FLAG_NORMAL, &btn_margin);
  gtk_style_context_get_border (button_context, GTK_STATE_FLAG_NORMAL, &btn_border);
  gtk_style_context_get_padding (button_context, GTK_STATE_FLAG_NORMAL, &btn_padding);
  gtk_style_context_get (button_context, GTK_STATE_FLAG_NORMAL,
                         "min-height", &btn_min_height, NULL);
  gtk_style_context_get (button_context, GTK_STATE_FLAG_NORMAL,
                         "min-width", &btn_min_width, NULL);

  /* Mirror GtkHeaderBar's height calculation: max(headerbar min-height, button
   * content + button margins) plus the headerbar's own padding and border. */
  btn_content_h = btn_min_height + btn_border.top + btn_border.bottom
                  + btn_padding.top + btn_padding.bottom;
  btn_content_w = btn_min_width + btn_border.left + btn_border.right
                  + btn_padding.left + btn_padding.right;
  content = MAX (hb_min_height, btn_content_h + btn_margin.top + btn_margin.bottom);
  headerbar_height = content + hb_padding.top + hb_padding.bottom
                     + hb_border.top + hb_border.bottom;
  button_height = btn_content_h;
  button_width = btn_content_w;
}
