/*
 *  Copyright © 2000-2003 Marco Pesenti Gritti
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#if !defined (__EPHY_EPIPHANY_H_INSIDE__) && !defined (EPIPHANY_COMPILATION)
#error "Only <epiphany/epiphany.h> can be included directly."
#endif

#ifndef EPHY_WINDOW_H
#define EPHY_WINDOW_H

#include "ephy-web-view.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EPHY_TYPE_WINDOW	(ephy_window_get_type ())
#define EPHY_WINDOW(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), EPHY_TYPE_WINDOW, EphyWindow))
#define EPHY_WINDOW_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), EPHY_TYPE_WINDOW, EphyWindowClass))
#define EPHY_IS_WINDOW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), EPHY_TYPE_WINDOW))
#define EPHY_IS_WINDOW_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), EPHY_TYPE_WINDOW))
#define EPHY_WINDOW_GET_CLASS(o)(G_TYPE_INSTANCE_GET_CLASS ((o), EPHY_TYPE_WINDOW, EphyWindowClass))

typedef struct _EphyWindowClass		EphyWindowClass;
typedef struct _EphyWindow		EphyWindow;
typedef struct _EphyWindowPrivate	EphyWindowPrivate;

struct _EphyWindow
{
	GtkApplicationWindow parent;

	/*< private >*/
	EphyWindowPrivate *priv;
};

struct _EphyWindowClass
{
	GtkApplicationWindowClass parent_class;
};

GType		  ephy_window_get_type		  (void);

EphyWindow	 *ephy_window_new		  (void);

EphyWindow	 *ephy_window_new_with_chrome	  (EphyWebViewChrome chrome,
						   gboolean is_popup);

GtkUIManager	 *ephy_window_get_ui_manager	  (EphyWindow *window);

GtkWidget	 *ephy_window_get_notebook	  (EphyWindow *window);

void		  ephy_window_load_url		  (EphyWindow *window,
						   const char *url);

void		  ephy_window_set_zoom		  (EphyWindow *window,
						   float zoom);

void		  ephy_window_activate_location	  (EphyWindow *window);
const char       *ephy_window_get_location        (EphyWindow *window);

gboolean          ephy_window_close               (EphyWindow *window);

gboolean      ephy_window_is_on_current_workspace (EphyWindow *window);

G_END_DECLS

#endif
