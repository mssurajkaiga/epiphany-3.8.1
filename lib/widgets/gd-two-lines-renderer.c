/*
 * Copyright (c) 2011 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

#include "gd-two-lines-renderer.h"
#include <string.h>

G_DEFINE_TYPE (GdTwoLinesRenderer, gd_two_lines_renderer, GTK_TYPE_CELL_RENDERER_TEXT)

struct _GdTwoLinesRendererPrivate {
  gchar *line_two;
  gint text_lines;
};

enum {
  PROP_TEXT_LINES = 1,
  PROP_LINE_TWO,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

static PangoLayout *
create_layout_with_attrs (GtkWidget *widget,
                          GdTwoLinesRenderer *self,
                          PangoEllipsizeMode ellipsize)
{
  PangoLayout *layout;
  gint wrap_width;
  PangoWrapMode wrap_mode;
  PangoAlignment alignment;

  g_object_get (self,
                "wrap-width", &wrap_width,
                "wrap-mode", &wrap_mode,
                "alignment", &alignment,
                NULL);

  layout = pango_layout_new (gtk_widget_get_pango_context (widget));

  pango_layout_set_ellipsize (layout, ellipsize);
  pango_layout_set_wrap (layout, wrap_mode);
  pango_layout_set_alignment (layout, alignment);

  if (wrap_width != -1)
    pango_layout_set_width (layout, wrap_width * PANGO_SCALE);

  return layout;
}

static void
gd_two_lines_renderer_prepare_layouts (GdTwoLinesRenderer *self,
                                       GtkWidget *widget,
                                       PangoLayout **layout_one,
                                       PangoLayout **layout_two)
{
  PangoLayout *line_one;
  PangoLayout *line_two = NULL;
  gchar *text = NULL;

  g_object_get (self,
                "text", &text,
                NULL);

  line_one = create_layout_with_attrs (widget, self, PANGO_ELLIPSIZE_MIDDLE);

  if (self->priv->line_two == NULL ||
      g_strcmp0 (self->priv->line_two, "") == 0)
    {
      pango_layout_set_height (line_one, - (self->priv->text_lines));

      if (text != NULL)
        pango_layout_set_text (line_one, text, -1);
    }
  else
    {
      line_two = create_layout_with_attrs (widget, self, PANGO_ELLIPSIZE_END);

      pango_layout_set_height (line_one, - (self->priv->text_lines - 1));
      pango_layout_set_height (line_two, -1);
      pango_layout_set_text (line_two, self->priv->line_two, -1);

      if (text != NULL)
        pango_layout_set_text (line_one, text, -1);
    }

  if (layout_one)
    *layout_one = line_one;
  if (layout_two)
    *layout_two = line_two;

  g_free (text);
}

static void
gd_two_lines_renderer_get_size (GtkCellRenderer *cell,
                                GtkWidget *widget,
                                PangoLayout *layout_1,
                                PangoLayout *layout_2,
                                gint *width,
                                gint *height,
                                const GdkRectangle *cell_area,
                                gint *x_offset_1,
                                gint *x_offset_2,
                                gint *y_offset)
{
  GdTwoLinesRenderer *self = GD_TWO_LINES_RENDERER (cell);
  gint xpad, ypad;
  PangoLayout *layout_one, *layout_two;
  GdkRectangle layout_one_rect, layout_two_rect, layout_union;

  if (layout_1 == NULL)
    {
      gd_two_lines_renderer_prepare_layouts (self, widget, &layout_one, &layout_two);
    }
  else
    {
      layout_one = g_object_ref (layout_1);

      if (layout_2 != NULL)
        layout_two = g_object_ref (layout_2);
      else
        layout_two = NULL;
    }

  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);
  pango_layout_get_pixel_extents (layout_one, NULL, (PangoRectangle *) &layout_one_rect);

  if (layout_two != NULL)
    {
      pango_layout_get_pixel_extents (layout_two, NULL, (PangoRectangle *) &layout_two_rect);

      layout_union.width = MAX(layout_one_rect.width, layout_two_rect.width);
      layout_union.height = layout_one_rect.height + layout_two_rect.height;
    }
  else
    {
      layout_union = layout_one_rect;
    }

  if (cell_area)
    {
      gfloat xalign, yalign;

      gtk_cell_renderer_get_alignment (cell, &xalign, &yalign);

      layout_union.width  = MIN (layout_union.width, cell_area->width - 2 * xpad);
      layout_union.height = MIN (layout_union.height, cell_area->height - 2 * ypad);

      if (x_offset_1)
	{
	  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL &&
              pango_layout_get_alignment (layout_one) != PANGO_ALIGN_CENTER)
	    *x_offset_1 = (1.0 - xalign) * (cell_area->width - (layout_one_rect.width + (2 * xpad)));
	  else 
	    *x_offset_1 = xalign * (cell_area->width - (layout_one_rect.width + (2 * xpad)));

          *x_offset_1 = MAX (*x_offset_1, 0);
	}
      if (x_offset_2)
        {
          if (layout_two != NULL)
            {
              if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL &&
                  pango_layout_get_alignment (layout_two) != PANGO_ALIGN_CENTER)
                *x_offset_2 = (1.0 - xalign) * (cell_area->width - (layout_two_rect.width + (2 * xpad)));
              else 
                *x_offset_2 = xalign * (cell_area->width - (layout_two_rect.width + (2 * xpad)));

              *x_offset_2 = MAX (*x_offset_2, 0);
            }
          else
            {
              *x_offset_2 = 0;
            }
        }

      if (y_offset)
	{
	  *y_offset = yalign * (cell_area->height - (layout_union.height + (2 * ypad)));
	  *y_offset = MAX (*y_offset, 0);
	}
    }
  else
    {
      if (x_offset_1) *x_offset_1 = 0;
      if (x_offset_2) *x_offset_2 = 0;
      if (y_offset) *y_offset = 0;
    }

  g_clear_object (&layout_one);
  g_clear_object (&layout_two);

  if (height)
    *height = ypad * 2 + layout_union.height;

  if (width)
    *width = xpad * 2 + layout_union.width;
}

static void
gd_two_lines_renderer_render (GtkCellRenderer      *cell,
                              cairo_t              *cr,
                              GtkWidget            *widget,
                              const GdkRectangle   *background_area,
                              const GdkRectangle   *cell_area,
                              GtkCellRendererState  flags)
{
  GdTwoLinesRenderer *self = GD_TWO_LINES_RENDERER (cell);
  GtkStyleContext *context;
  gint line_one_height;
  GtkStateFlags state;
  GdkRectangle render_area = *cell_area;
  gint xpad, ypad, x_offset_1, x_offset_2, y_offset;
  PangoLayout *layout_one, *layout_two;

  context = gtk_widget_get_style_context (widget);
  gd_two_lines_renderer_prepare_layouts (self, widget, &layout_one, &layout_two);
  gd_two_lines_renderer_get_size (cell, widget,
                                  layout_one, layout_two,
                                  NULL, NULL,
                                  cell_area,
                                  &x_offset_1, &x_offset_2, &y_offset);

  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

  render_area.x += xpad + x_offset_1;
  render_area.y += ypad;

  pango_layout_set_width (layout_one,
                          (cell_area->width - x_offset_1 - 2 * xpad) * PANGO_SCALE);

  gtk_render_layout (context, cr,
                     render_area.x,
                     render_area.y,
                     layout_one);

  if (layout_two != NULL)
    {
      pango_layout_get_pixel_size (layout_one,
                                   NULL, &line_one_height);

      gtk_style_context_save (context);
      gtk_style_context_add_class (context, "dim-label");

      state = gtk_cell_renderer_get_state (cell, widget, flags);
      gtk_style_context_set_state (context, state);

      render_area.x += - x_offset_1 + x_offset_2;
      render_area.y += line_one_height;
      pango_layout_set_width (layout_two,
                              (cell_area->width - x_offset_2 - 2 * xpad) * PANGO_SCALE);

      gtk_render_layout (context, cr,
                         render_area.x,
                         render_area.y,
                         layout_two);

      gtk_style_context_restore (context);
    }

  g_clear_object (&layout_one);
  g_clear_object (&layout_two);
}

static void
gd_two_lines_renderer_get_preferred_width (GtkCellRenderer *cell,
                                           GtkWidget       *widget,
                                           gint            *minimum_size,
                                           gint            *natural_size)
{
  PangoContext *context;
  PangoFontMetrics *metrics;
  const PangoFontDescription *font_desc;
  GtkStyleContext *style_context;
  gint nat_width, min_width;
  gint xpad, char_width, wrap_width, text_width;
  gint width_chars, ellipsize_chars;

  g_object_get (cell,
                "xpad", &xpad,
                "width-chars", &width_chars,
                "wrap-width", &wrap_width,
                NULL);
  style_context = gtk_widget_get_style_context (widget);
  gtk_cell_renderer_get_padding (cell, &xpad, NULL);

  gd_two_lines_renderer_get_size (cell, widget,
                                  NULL, NULL,
                                  &text_width, NULL,
                                  NULL, 
                                  NULL, NULL, NULL);

  /* Fetch the average size of a charachter */
  context = gtk_widget_get_pango_context (widget);
  gtk_style_context_get (style_context, 0, "font", &font_desc, NULL);
  metrics = pango_context_get_metrics (context, font_desc,
                                       pango_context_get_language (context));

  char_width = pango_font_metrics_get_approximate_char_width (metrics);

  pango_font_metrics_unref (metrics);

  /* enforce minimum width for ellipsized labels at ~3 chars */
  ellipsize_chars = 3;

  /* If no width-chars set, minimum for wrapping text will be the wrap-width */
  if (wrap_width > -1)
    min_width = xpad * 2 + MIN (text_width, wrap_width);
  else
    min_width = xpad * 2 +
      MIN (text_width,
           (PANGO_PIXELS (char_width) * MAX (width_chars, ellipsize_chars)));

  if (width_chars > 0)
    nat_width = xpad * 2 +
      MAX ((PANGO_PIXELS (char_width) * width_chars), text_width);
  else
    nat_width = xpad * 2 + text_width;

  nat_width = MAX (nat_width, min_width);

  if (minimum_size)
    *minimum_size = min_width;

  if (natural_size)
    *natural_size = nat_width;
}

static void
gd_two_lines_renderer_get_preferred_height_for_width (GtkCellRenderer *cell,
                                                      GtkWidget       *widget,
                                                      gint             width,
                                                      gint            *minimum_size,
                                                      gint            *natural_size)
{
  gint text_height;
  gint ypad;

  gd_two_lines_renderer_get_size (cell, widget,
                                  NULL, NULL,
                                  NULL, &text_height,
                                  NULL, 
                                  NULL, NULL, NULL);

  gtk_cell_renderer_get_padding (cell, NULL, &ypad);
  text_height += 2 * ypad;

  if (minimum_size != NULL)
    *minimum_size = text_height;

  if (natural_size != NULL)
    *natural_size = text_height;
}

static void
gd_two_lines_renderer_get_preferred_height (GtkCellRenderer *cell,
                                            GtkWidget       *widget,
                                            gint            *minimum_size,
                                            gint            *natural_size)
{
  gint min_width;

  gtk_cell_renderer_get_preferred_width (cell, widget, &min_width, NULL);
  gd_two_lines_renderer_get_preferred_height_for_width (cell, widget, min_width,
                                                        minimum_size, natural_size);
}

static void
gd_two_lines_renderer_get_aligned_area (GtkCellRenderer      *cell,
                                        GtkWidget            *widget,
                                        GtkCellRendererState  flags,
                                        const GdkRectangle   *cell_area,
                                        GdkRectangle         *aligned_area)
{
  gint x_offset_1, x_offset_2, y_offset;

  gd_two_lines_renderer_get_size (cell, widget,
                                  NULL, NULL,
                                  &aligned_area->width, &aligned_area->height,
                                  cell_area, 
                                  &x_offset_1, &x_offset_2, &y_offset);

  aligned_area->x = cell_area->x + MAX (x_offset_1, x_offset_2);
  aligned_area->y = cell_area->y;
}

static void
gd_two_lines_renderer_set_line_two (GdTwoLinesRenderer *self,
                                    const gchar *line_two)
{
  if (g_strcmp0 (self->priv->line_two, line_two) == 0)
    return;

  g_free (self->priv->line_two);
  self->priv->line_two = g_strdup (line_two);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_LINE_TWO]);
}

static void
gd_two_lines_renderer_set_text_lines (GdTwoLinesRenderer *self,
                                      gint text_lines)
{
  if (self->priv->text_lines == text_lines)
    return;

  self->priv->text_lines = text_lines;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TEXT_LINES]);
}

static void
gd_two_lines_renderer_set_property (GObject    *object,
                                    guint       property_id,
                                    const GValue     *value,
                                    GParamSpec *pspec)
{
  GdTwoLinesRenderer *self = GD_TWO_LINES_RENDERER (object);

  switch (property_id)
    {
    case PROP_TEXT_LINES:
      gd_two_lines_renderer_set_text_lines (self, g_value_get_int (value));
      break;
    case PROP_LINE_TWO:
      gd_two_lines_renderer_set_line_two (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gd_two_lines_renderer_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GdTwoLinesRenderer *self = GD_TWO_LINES_RENDERER (object);

  switch (property_id)
    {
    case PROP_TEXT_LINES:
      g_value_set_int (value, self->priv->text_lines);
      break;
    case PROP_LINE_TWO:
      g_value_set_string (value, self->priv->line_two);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gd_two_lines_renderer_finalize (GObject *object)
{
  GdTwoLinesRenderer *self = GD_TWO_LINES_RENDERER (object);

  g_free (self->priv->line_two);

  G_OBJECT_CLASS (gd_two_lines_renderer_parent_class)->finalize (object);
}

static void
gd_two_lines_renderer_class_init (GdTwoLinesRendererClass *klass)
{
  GtkCellRendererClass *cclass = GTK_CELL_RENDERER_CLASS (klass);
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  cclass->render = gd_two_lines_renderer_render;
  cclass->get_preferred_width = gd_two_lines_renderer_get_preferred_width;
  cclass->get_preferred_height = gd_two_lines_renderer_get_preferred_height;
  cclass->get_preferred_height_for_width = gd_two_lines_renderer_get_preferred_height_for_width;
  cclass->get_aligned_area = gd_two_lines_renderer_get_aligned_area;

  oclass->set_property = gd_two_lines_renderer_set_property;
  oclass->get_property = gd_two_lines_renderer_get_property;
  oclass->finalize = gd_two_lines_renderer_finalize;
  
  properties[PROP_TEXT_LINES] =
    g_param_spec_int ("text-lines",
                      "Lines of text",
                      "The total number of lines to be displayed",
                      2, G_MAXINT, 2,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_LINE_TWO] =
    g_param_spec_string ("line-two",
                         "Second line",
                         "Second line",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_type_class_add_private (klass, sizeof (GdTwoLinesRendererPrivate));
  g_object_class_install_properties (oclass, NUM_PROPERTIES, properties);
}

static void
gd_two_lines_renderer_init (GdTwoLinesRenderer *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GD_TYPE_TWO_LINES_RENDERER,
                                            GdTwoLinesRendererPrivate);
}

GtkCellRenderer *
gd_two_lines_renderer_new (void)
{
  return g_object_new (GD_TYPE_TWO_LINES_RENDERER, NULL);
}
