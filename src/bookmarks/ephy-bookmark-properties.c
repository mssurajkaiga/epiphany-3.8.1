/*
 *  Copyright © 2002 Marco Pesenti Gritti <mpeseng@tin.it>
 *  Copyright © 2005, 2006 Peter A. Harvey
 *  Copyright © 2006 Christian Persch
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#include "config.h"

#include "ephy-bookmark-properties.h"

#include "ephy-bookmarks-ui.h"
#include "ephy-topics-entry.h"
#include "ephy-topics-palette.h"
#include "ephy-node-common.h"
#include "ephy-debug.h"
#include "ephy-shell.h"
#include "ephy-initial-state.h"
#include "ephy-gui.h"
#include "ephy-dnd.h"
#include "ephy-prefs.h"
#include "ephy-settings.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <string.h>

static const GtkTargetEntry dest_drag_types[] = {
  {EPHY_DND_URL_TYPE, 0, 0},
};

#define EPHY_BOOKMARK_PROPERTIES_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), EPHY_TYPE_BOOKMARK_PROPERTIES, EphyBookmarkPropertiesPrivate))

struct _EphyBookmarkPropertiesPrivate
{
	EphyBookmarks *bookmarks;
	EphyNode *bookmark;
	gboolean creating;
	
	gint duplicate_count;
	gint duplicate_idle;
	
	GtkWidget *warning;
	GtkWidget *entry;
};

enum
{
	PROP_0,
	PROP_BOOKMARKS,
	PROP_BOOKMARK,
	PROP_CREATING
};

G_DEFINE_TYPE (EphyBookmarkProperties, ephy_bookmark_properties, GTK_TYPE_DIALOG)

static gboolean
update_warning (EphyBookmarkProperties *properties)
{
	EphyBookmarkPropertiesPrivate *priv = properties->priv;
	char *label;

	priv->duplicate_idle = 0;	
	priv->duplicate_count = ephy_bookmarks_get_similar
	  (priv->bookmarks, priv->bookmark, NULL, NULL);
	
        /* Translators: This string is used when counting bookmarks that
         * are similar to each other */
	label = g_strdup_printf (ngettext("%d _Similar", "%d _Similar", priv->duplicate_count), priv->duplicate_count);
	gtk_button_set_label (GTK_BUTTON (priv->warning), label);
	g_free (label);

	g_object_set (priv->warning, "sensitive", priv->duplicate_count > 0, NULL);

	return FALSE;
}

static void
update_warning_idle (EphyBookmarkProperties *properties)
{
	EphyBookmarkPropertiesPrivate *priv = properties->priv;
	
	if(priv->duplicate_idle != 0)
	{
		g_source_remove (priv->duplicate_idle);
	}
	
	priv->duplicate_idle = g_timeout_add 
	  (500, (GSourceFunc)update_warning, properties);
}

static void
node_added_cb (EphyNode *bookmarks,
	       EphyNode *bookmark,
	       EphyBookmarkProperties *properties)
{
	update_warning_idle (properties);
}

static void
node_changed_cb (EphyNode *bookmarks,
		 EphyNode *bookmark,
		 guint property,
		 EphyBookmarkProperties *properties)
{
	if (property == EPHY_NODE_BMK_PROP_LOCATION) 
	{
		update_warning_idle (properties);
	}
}

static void
node_removed_cb (EphyNode *bookmarks,
		 EphyNode *bookmark,
		 guint index,
		 EphyBookmarkProperties *properties)
{
	update_warning_idle (properties);
}

static void
node_destroy_cb (EphyNode *bookmark,
		 GtkWidget *dialog)
{
	EPHY_BOOKMARK_PROPERTIES (dialog)->priv->creating = FALSE;
	gtk_widget_destroy (dialog);
}

static void
ephy_bookmark_properties_set_bookmark (EphyBookmarkProperties *properties,
				       EphyNode *bookmark)
{
	EphyBookmarkPropertiesPrivate *priv = properties->priv;

	LOG ("Set bookmark");
	
	if (priv->bookmark)
	{
		ephy_node_signal_disconnect_object (priv->bookmark,
						    EPHY_NODE_DESTROY,
						    (EphyNodeCallback) node_destroy_cb,
						    G_OBJECT (properties));
	}

	priv->bookmark = bookmark;

	ephy_node_signal_connect_object (priv->bookmark,
					 EPHY_NODE_DESTROY,
					 (EphyNodeCallback) node_destroy_cb,
					 G_OBJECT (properties));
}

static void
similar_merge_cb (GtkMenuItem *item,
		  EphyBookmarkProperties *properties)
{
	EphyBookmarkPropertiesPrivate *priv = properties->priv;
	GPtrArray *topics;
	EphyNode *node, *topic;
	gint i, j, priority;

	GPtrArray *identical = g_ptr_array_new ();
	
	ephy_bookmarks_get_similar
	  (priv->bookmarks, priv->bookmark, identical, NULL);
	
	node = ephy_bookmarks_get_keywords (priv->bookmarks);
	topics = ephy_node_get_children (node);

	for (i = 0; i < identical->len; i++)
	{	
		node = g_ptr_array_index (identical, i);
		for (j = 0; j < topics->len; j++)
		{
			topic = g_ptr_array_index (topics, j);
			
			priority = ephy_node_get_property_int
			  (topic, EPHY_NODE_KEYWORD_PROP_PRIORITY);
			if (priority != EPHY_NODE_NORMAL_PRIORITY)
			  continue;

			if (ephy_node_has_child (topic, node))
			{
				ephy_bookmarks_set_keyword
				  (priv->bookmarks, topic, priv->bookmark);
			}
		}
		ephy_node_unref (node);
	}	
	
	g_ptr_array_free (identical, TRUE);
	
	update_warning (properties);
}

static void
similar_show_cb (GtkMenuItem *item,
		 EphyNode *node)
{
	ephy_bookmarks_ui_show_bookmark (node);
}

static void
similar_deactivate_cb (GtkMenuShell *ms,
		       GtkWidget *button)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
}

static void
similar_toggled_cb (GtkWidget *button,
		    EphyBookmarkProperties *properties)
{
	EphyBookmarkPropertiesPrivate *priv = properties->priv;
	EphyNode *node;
	GtkMenuShell *menu;
	GtkWidget *item, *image;
	GPtrArray *identical, *similar;
	char *label;
	gint i;
	
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
	{
		return;
	}
	
	identical = g_ptr_array_new ();
	similar = g_ptr_array_new ();

	ephy_bookmarks_get_similar (priv->bookmarks,
				    priv->bookmark,
				    identical,
				    similar);
	
	if (identical->len + similar->len > 0)
	{
		menu = GTK_MENU_SHELL (gtk_menu_new ());
		
		if (identical->len > 0)
		{
			label = g_strdup_printf (ngettext ("_Unify With %d Identical Bookmark",
							   "_Unify With %d Identical Bookmarks",
							   identical->len),
						 identical->len);
			item = gtk_image_menu_item_new_with_mnemonic (label);
			g_free (label);
			image = gtk_image_new_from_stock (GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU);
			gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
			g_signal_connect (item, "activate", G_CALLBACK (similar_merge_cb), properties);
			gtk_widget_show (image);
			gtk_widget_show (item);
			gtk_menu_shell_append (menu, item);
	
			item = gtk_separator_menu_item_new ();
			gtk_widget_show (item);
			gtk_menu_shell_append (menu, item);
	
			for (i = 0; i < identical->len; i++)
			{
				node = g_ptr_array_index (identical, i);
				label = g_strdup_printf (_("Show “%s”"),
							 ephy_node_get_property_string (node, EPHY_NODE_BMK_PROP_TITLE));
				item = gtk_image_menu_item_new_with_label (label);
				g_free (label);
				g_signal_connect (item, "activate", G_CALLBACK (similar_show_cb), node);
				gtk_widget_show (item);
				gtk_menu_shell_append (menu, item);
			}
		}
		
		if (identical->len > 0 && similar->len > 0)
		{
			item = gtk_separator_menu_item_new ();
			gtk_widget_show (item);
			gtk_menu_shell_append (menu, item);
		}
		
		if (similar->len > 0)
		{
			for (i = 0; i < similar->len; i++)
			{
				node = g_ptr_array_index (similar, i);
				label = g_strdup_printf (_("Show “%s”"),
							 ephy_node_get_property_string (node, EPHY_NODE_BMK_PROP_TITLE));
				item = gtk_image_menu_item_new_with_label (label);
				g_free (label);
				g_signal_connect (item, "activate", G_CALLBACK (similar_show_cb), node);
				gtk_widget_show (item);
				gtk_menu_shell_append (menu, item);
			}
		}
		
		g_signal_connect_object (menu, "deactivate",
					 G_CALLBACK (similar_deactivate_cb), button, 0);
		
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
				ephy_gui_menu_position_under_widget, button,
				1, gtk_get_current_event_time ());
	}
		
	g_ptr_array_free (similar, TRUE);
	g_ptr_array_free (identical, TRUE);
}

static gboolean
similar_release_cb (GtkWidget *button,
		    GdkEventButton *event,
		    EphyBookmarkProperties *properties)
{
	if (event->button == 1)
	{
		gtk_toggle_button_set_active
			(GTK_TOGGLE_BUTTON (button), FALSE);
	}

	return FALSE;
}

static gboolean
similar_press_cb (GtkWidget *button,
		  GdkEventButton *event,
		  EphyBookmarkProperties *properties)
{
	if (event->button == 1)
	{
		if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

			return TRUE;
		}
	}

	return FALSE;
}

static void
bookmark_properties_destroy_cb (GtkDialog *dialog,
				gpointer data)
{
	EphyBookmarkProperties *properties = EPHY_BOOKMARK_PROPERTIES (dialog);
	EphyBookmarkPropertiesPrivate *priv = properties->priv;

	if (priv->creating)
	{
		ephy_node_unref (priv->bookmark);
		priv->creating = FALSE;
	}
	
	if(priv->duplicate_idle != 0)
	{
		g_source_remove (priv->duplicate_idle);
		priv->duplicate_idle = 0;
	}
}

static void
bookmark_properties_response_cb (GtkDialog *dialog,
				 int response_id,
				 gpointer data)
{
	EphyBookmarkProperties *properties = EPHY_BOOKMARK_PROPERTIES (dialog);
	EphyBookmarkPropertiesPrivate *priv = properties->priv;

	switch (response_id)
	{
		case GTK_RESPONSE_HELP:
			ephy_gui_help (GTK_WIDGET (dialog),
				       "to-edit-bookmark-properties");
			return;
	 	case GTK_RESPONSE_ACCEPT:
			priv->creating = FALSE;
			break;
	 	default:
			break;
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
update_entry (EphyBookmarkProperties *props,
	      GtkWidget *entry,
	      guint prop)
{
	GValue value = { 0, };
	char *text;

	text = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
	g_value_init (&value, G_TYPE_STRING);
	g_value_take_string (&value, text);
	ephy_node_set_property (props->priv->bookmark,
				prop,
				&value);
	g_value_unset (&value);
}

static void
update_window_title (EphyBookmarkProperties *properties)
{
	EphyBookmarkPropertiesPrivate *priv = properties->priv;
	char *title;
	const char *tmp;

	tmp = ephy_node_get_property_string (priv->bookmark,
					     EPHY_NODE_BMK_PROP_TITLE);

	title = g_strdup_printf (_("“%s” Properties"), tmp);
	gtk_window_set_title (GTK_WINDOW (properties), title);
	g_free (title);
}

static void
title_entry_changed_cb (GtkWidget *entry,
			EphyBookmarkProperties *props)
{
	update_entry (props, entry, EPHY_NODE_BMK_PROP_TITLE);
	update_window_title (props);
}

static void
location_entry_changed_cb (GtkWidget *entry,
			   EphyBookmarkProperties *properties)
{
	EphyBookmarkPropertiesPrivate *priv = properties->priv;

	ephy_bookmarks_set_address (priv->bookmarks,
				    priv->bookmark,
				    gtk_entry_get_text (GTK_ENTRY (entry)));
}

static void
list_mapped_cb (GtkWidget *widget,
		 EphyBookmarkProperties *properties)
{
	GdkGeometry geometry;
	
	geometry.min_width = -1;
	geometry.min_height = 230;
	gtk_window_set_geometry_hints (GTK_WINDOW (properties),
				       widget, &geometry,
				       GDK_HINT_MIN_SIZE);
}

static void
list_unmapped_cb (GtkWidget *widget,
		 EphyBookmarkProperties *properties)
{
	GdkGeometry geometry;
	
	geometry.max_height = -1;
	geometry.max_width = G_MAXINT;
	gtk_window_set_geometry_hints (GTK_WINDOW (properties),	
				       GTK_WIDGET (properties),
				       &geometry, GDK_HINT_MAX_SIZE);
}

static void
ephy_bookmark_properties_init (EphyBookmarkProperties *properties)
{
	properties->priv = EPHY_BOOKMARK_PROPERTIES_GET_PRIVATE (properties);
}

static GObject *
ephy_bookmark_properties_constructor (GType type,
				      guint n_construct_properties,
				      GObjectConstructParam *construct_params)
{
	GObject *object;
	EphyBookmarkProperties *properties;
	EphyBookmarkPropertiesPrivate *priv;
	GtkWidget *widget, *grid, *label, *entry, *container;
	GtkWidget *content_area, *action_area;
	GtkWindow *window;
	GtkDialog *dialog;
	gboolean lockdown;
	const char *tmp;
	char *text;

	object = G_OBJECT_CLASS (ephy_bookmark_properties_parent_class)->constructor (type,
                                                                                      n_construct_properties,
                                                                                      construct_params);

	widget = GTK_WIDGET (object);
	window = GTK_WINDOW (object);
	dialog = GTK_DIALOG (object);
	properties = EPHY_BOOKMARK_PROPERTIES (object);
	priv = properties->priv;

	gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DIALOG);

	g_signal_connect (properties, "response",
			  G_CALLBACK (bookmark_properties_response_cb), properties);

	g_signal_connect (properties, "destroy",
			  G_CALLBACK (bookmark_properties_destroy_cb), properties);

	if (!priv->creating)
	{
		ephy_initial_state_add_window (widget,
                                               "bookmark_properties",
                                               290, 280, FALSE,
                                               EPHY_INITIAL_STATE_WINDOW_SAVE_POSITION |
                                               EPHY_INITIAL_STATE_WINDOW_SAVE_SIZE);
	}
	/* Lockdown */
	lockdown = g_settings_get_boolean (EPHY_SETTINGS_LOCKDOWN,
					   EPHY_PREFS_LOCKDOWN_BOOKMARK_EDITING);

	update_window_title (properties);
	content_area = gtk_dialog_get_content_area (dialog);

	gtk_container_set_border_width (GTK_CONTAINER (properties), 5);
	gtk_box_set_spacing (GTK_BOX (content_area), 2);

	grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
	gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
	gtk_container_set_border_width (GTK_CONTAINER (grid), 5);
	gtk_widget_show (grid);

	entry = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (entry), !lockdown);
	tmp = ephy_node_get_property_string (properties->priv->bookmark,
					     EPHY_NODE_BMK_PROP_TITLE);
	gtk_entry_set_text (GTK_ENTRY (entry), tmp);
	g_signal_connect (entry, "changed",
			  G_CALLBACK (title_entry_changed_cb), properties);
	gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
	gtk_widget_set_size_request (entry, 200, -1);
	gtk_widget_show (entry);
	label = gtk_label_new_with_mnemonic (_("_Title:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
	gtk_widget_show (label);
	gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), entry, 1, 0, 1, 1);
	gtk_widget_set_hexpand (entry, TRUE);

	entry = gtk_entry_new ();
	gtk_editable_set_editable (GTK_EDITABLE (entry), !lockdown);
	tmp = ephy_node_get_property_string (properties->priv->bookmark,
					     EPHY_NODE_BMK_PROP_LOCATION);
	gtk_entry_set_text (GTK_ENTRY (entry), tmp);
	g_signal_connect (entry, "changed",
			  G_CALLBACK (location_entry_changed_cb), properties);
	gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
	gtk_widget_show (entry);
	label = gtk_label_new_with_mnemonic (_("A_ddress:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
	gtk_widget_show (label);
	gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), entry, 1, 1, 1, 1);
	gtk_widget_set_hexpand (entry, TRUE);

	entry = ephy_topics_entry_new (priv->bookmarks, priv->bookmark);
	gtk_editable_set_editable (GTK_EDITABLE (entry), !lockdown);
	priv->entry = entry;
	gtk_widget_show (entry);
	label = gtk_label_new_with_mnemonic(_("T_opics:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
	gtk_widget_show (label);
	gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), entry, 1, 2, 1, 1);
	gtk_widget_set_hexpand (entry, TRUE);

	widget = ephy_topics_palette_new (priv->bookmarks, priv->bookmark);
	container = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
				  "hadjustment", NULL,
				  "vadjustment", NULL,
				  "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
				  "vscrollbar_policy", GTK_POLICY_AUTOMATIC,
				  "shadow_type", GTK_SHADOW_IN,
				  NULL);
	gtk_container_add (GTK_CONTAINER (container), widget);
	gtk_widget_show (widget);
	gtk_widget_set_sensitive (container, !lockdown);
	gtk_widget_show (container);
	g_signal_connect (container, "map", G_CALLBACK (list_mapped_cb), properties);
	g_signal_connect (container, "unmap", G_CALLBACK (list_unmapped_cb), properties);

	widget = gtk_expander_new (_("Sho_w all topics"));
	gtk_expander_set_use_underline (GTK_EXPANDER (widget), TRUE);
	ephy_initial_state_add_expander (widget, "bookmark_properties_list", FALSE);
	gtk_container_add (GTK_CONTAINER (widget), container);
	gtk_widget_show (widget);
	gtk_grid_attach (GTK_GRID (grid), widget, 1, 3, 1, 1);
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_widget_set_vexpand (widget, TRUE);
	
	gtk_box_pack_start (GTK_BOX (content_area), grid, TRUE, TRUE, 0);
	
	priv->warning = gtk_toggle_button_new ();
	gtk_button_set_focus_on_click (GTK_BUTTON (priv->warning), FALSE);
	gtk_button_set_use_underline (GTK_BUTTON (priv->warning), TRUE);
	text = g_strdup_printf (ngettext("%d _Similar", "%d _Similar", 0), 0);
	gtk_button_set_label (GTK_BUTTON (priv->warning), text);
	g_free (text);
	widget = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (widget);
	gtk_button_set_image (GTK_BUTTON (priv->warning), widget);
	g_object_set (priv->warning, "sensitive", FALSE, NULL);
	gtk_widget_show (priv->warning);
	g_signal_connect (priv->warning, "toggled",
			  G_CALLBACK (similar_toggled_cb), properties);
	g_signal_connect (priv->warning, "button-press-event",
			  G_CALLBACK (similar_press_cb), properties);
	g_signal_connect (priv->warning, "button-release-event",
			  G_CALLBACK (similar_release_cb), properties);
		
	gtk_dialog_add_button (dialog,
			       GTK_STOCK_HELP,
			       GTK_RESPONSE_HELP);

	action_area = gtk_dialog_get_action_area (dialog);
	
	gtk_box_pack_end (GTK_BOX (action_area),
			  priv->warning, FALSE, TRUE, 0);
	gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (action_area),
					    priv->warning, TRUE);
	
	if (priv->creating)
	{
		gtk_dialog_add_button (dialog,
				       GTK_STOCK_CANCEL,
				       GTK_RESPONSE_CANCEL);
		gtk_dialog_add_button (dialog,
				       GTK_STOCK_ADD,
				       GTK_RESPONSE_ACCEPT);
		gtk_dialog_set_default_response (dialog, GTK_RESPONSE_ACCEPT);
	}
	else
	{
		gtk_dialog_add_button (dialog,
				       GTK_STOCK_CLOSE,
				       GTK_RESPONSE_CLOSE);
		gtk_dialog_set_default_response (dialog, GTK_RESPONSE_CLOSE);
	}

	update_warning (properties);
	
	return object;
}

static void
ephy_bookmark_properties_set_property (GObject *object,
				       guint prop_id,
				       const GValue *value,
				       GParamSpec *pspec)
{
	EphyBookmarkProperties *properties = EPHY_BOOKMARK_PROPERTIES (object);
	EphyBookmarkPropertiesPrivate *priv = properties->priv;
	EphyNode *bookmarks;

	switch (prop_id)
	{
		case PROP_BOOKMARKS:
			priv->bookmarks = g_value_get_object (value);
			bookmarks = ephy_bookmarks_get_bookmarks (priv->bookmarks);
			ephy_node_signal_connect_object (bookmarks,
							 EPHY_NODE_CHILD_ADDED,
							 (EphyNodeCallback) node_added_cb,
							 object);
			ephy_node_signal_connect_object (bookmarks,
							 EPHY_NODE_CHILD_REMOVED,
							 (EphyNodeCallback) node_removed_cb,
							 object);
			ephy_node_signal_connect_object (bookmarks,
							 EPHY_NODE_CHILD_CHANGED,
							 (EphyNodeCallback) node_changed_cb,
							 object);
			break;
		case PROP_BOOKMARK:
			ephy_bookmark_properties_set_bookmark
				(properties, g_value_get_pointer (value));
			break;
		case PROP_CREATING:
			priv->creating = g_value_get_boolean (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
ephy_bookmark_properties_get_property (GObject *object,
				       guint prop_id,
				       GValue *value,
				       GParamSpec *pspec)
{
	EphyBookmarkProperties *properties = EPHY_BOOKMARK_PROPERTIES (object);

	switch (prop_id)
	{
		case PROP_BOOKMARK:
			g_value_set_object (value, properties);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
ephy_bookmark_properties_class_init (EphyBookmarkPropertiesClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->constructor = ephy_bookmark_properties_constructor;
	object_class->set_property = ephy_bookmark_properties_set_property;
	object_class->get_property = ephy_bookmark_properties_get_property;

	g_object_class_install_property (object_class,
					 PROP_BOOKMARKS,
					 g_param_spec_object ("bookmarks",
							      "bookmarks",
							      "bookmarks",
							      EPHY_TYPE_BOOKMARKS,
							      G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
					 PROP_BOOKMARK,
					 g_param_spec_pointer ("bookmark",
							       "bookmark",
							       "bookmark",
							       G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
					 PROP_CREATING,
					 g_param_spec_boolean ("creating",
							       "creating",
							       "creating",
							       FALSE,
							       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private (object_class, sizeof (EphyBookmarkPropertiesPrivate));
}

/* public API */

GtkWidget *
ephy_bookmark_properties_new (EphyBookmarks *bookmarks,
			      EphyNode *bookmark,
			      gboolean creating)
{
	g_assert (bookmarks != NULL);

	return GTK_WIDGET (g_object_new	(EPHY_TYPE_BOOKMARK_PROPERTIES,
			   		 "bookmarks", bookmarks,
					 "bookmark", bookmark,
					 "creating", creating,
					 NULL));
}

EphyNode *
ephy_bookmark_properties_get_node (EphyBookmarkProperties *properties)
{
	return properties->priv->bookmark;
}
