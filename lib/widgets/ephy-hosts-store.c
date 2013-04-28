/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/* vim: set sw=2 ts=2 sts=2 et: */
/*
 *  Copyright © 2011, 2012 Igalia S.L.
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

#include "config.h"

#include "ephy-embed-prefs.h"
#include "ephy-favicon-helpers.h"
#include "ephy-hosts-store.h"

#include <glib/gi18n.h>
#ifdef HAVE_WEBKIT2
#include <libsoup/soup.h>
#endif

G_DEFINE_TYPE (EphyHostsStore, ephy_hosts_store, GTK_TYPE_LIST_STORE)

typedef struct {
  GtkListStore *model;
  GtkTreeRowReference *row_reference;
} IconLoadData;

static void
async_update_favicon_icon (GObject *source, GAsyncResult *result, gpointer user_data)
{
  GtkTreeIter iter;
  GtkTreePath *path;
  IconLoadData *data = (IconLoadData *)user_data;
  WebKitFaviconDatabase *database;
  GdkPixbuf *favicon = NULL;
#ifdef HAVE_WEBKIT2
  cairo_surface_t *icon_surface;
#endif

  database = WEBKIT_FAVICON_DATABASE (source);

#ifdef HAVE_WEBKIT2
  icon_surface = webkit_favicon_database_get_favicon_finish (database, result, NULL);

  if (icon_surface) {
    favicon = ephy_pixbuf_get_from_surface_scaled (icon_surface, FAVICON_SIZE, FAVICON_SIZE);
    cairo_surface_destroy (icon_surface);
  }
#else
  favicon = webkit_favicon_database_get_favicon_pixbuf_finish (database, result, NULL);
#endif

  if (favicon) {
    /* The completion model might have changed its contents */
    if (gtk_tree_row_reference_valid (data->row_reference)) {
      path = gtk_tree_row_reference_get_path (data->row_reference);
      gtk_tree_model_get_iter (GTK_TREE_MODEL (data->model), &iter, path);
      gtk_tree_path_free (path);
      gtk_list_store_set (data->model, &iter,
                          EPHY_HOSTS_STORE_COLUMN_FAVICON, favicon, -1);
    }
    g_object_unref (favicon);
  }

  g_object_unref (data->model);
  gtk_tree_row_reference_free (data->row_reference);
  g_slice_free (IconLoadData, data);
}

static void
icon_changed_cb (WebKitFaviconDatabase *database,
                 const char *page_uri,
#ifdef HAVE_WEBKIT2
                 const char *favicon_uri,
#endif
                 GtkTreeModel *model)
{
  GtkTreeIter iter;
  int cmp;
  char *host_address;
  gboolean done, valid;
  SoupURI *uri;

  valid = gtk_tree_model_get_iter_first (model, &iter);

  /* If the page_uri has a path, this icon is not for a host, so it
     can be skipped. */
  uri = soup_uri_new (page_uri);
  done = strcmp (soup_uri_get_path (uri), "/") != 0;
  soup_uri_free (uri);

  while (valid && !done) {
    gtk_tree_model_get (model, &iter,
                        EPHY_HOSTS_STORE_COLUMN_ADDRESS, &host_address, -1);
    cmp = g_strcmp0 (host_address, page_uri);
    g_free (host_address);

    if (cmp == 0) {
#ifdef HAVE_WEBKIT2
      IconLoadData *data;
      GtkTreePath *path;

      data = g_slice_new (IconLoadData);
      data->model = GTK_LIST_STORE (g_object_ref (model));
      path = gtk_tree_model_get_path (model, &iter);
      data->row_reference = gtk_tree_row_reference_new (model, path);
      gtk_tree_path_free (path);

      webkit_favicon_database_get_favicon (database,
                                           page_uri,
                                           0,
                                           async_update_favicon_icon,
                                           data);
#else
      GdkPixbuf *favicon = webkit_favicon_database_try_get_favicon_pixbuf (database,
                                                                           page_uri,
                                                                           FAVICON_SIZE,
                                                                           FAVICON_SIZE);
      if (favicon) {
        gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                            EPHY_HOSTS_STORE_COLUMN_FAVICON, favicon,
                            -1);
        g_object_unref (favicon);
      }

#endif
    }
    valid = gtk_tree_model_iter_next (model, &iter);

    /* Since the list is sorted alphanumerically, if the result of the
       comparison is > 0, there is no point in searching any
       further. */
    done = cmp >= 0;
  }
}

static void
ephy_hosts_store_finalize (GObject *object)
{
  EphyHostsStore *store = EPHY_HOSTS_STORE (object);
  WebKitFaviconDatabase *database;

#ifdef HAVE_WEBKIT2
  database = webkit_web_context_get_favicon_database (webkit_web_context_get_default ());
#else
  database = webkit_get_favicon_database ();
#endif

  g_signal_handlers_disconnect_by_func (database, icon_changed_cb, store);

  G_OBJECT_CLASS (ephy_hosts_store_parent_class)->finalize (object);
}

static void
ephy_hosts_store_class_init (EphyHostsStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ephy_hosts_store_finalize;
}

static void
ephy_hosts_store_init (EphyHostsStore *self)
{
  GType types[EPHY_HOSTS_STORE_N_COLUMNS];

  types[EPHY_HOSTS_STORE_COLUMN_ID]          = G_TYPE_INT;
  types[EPHY_HOSTS_STORE_COLUMN_TITLE]       = G_TYPE_STRING;
  types[EPHY_HOSTS_STORE_COLUMN_ADDRESS]     = G_TYPE_STRING;
  types[EPHY_HOSTS_STORE_COLUMN_VISIT_COUNT] = G_TYPE_INT;
  types[EPHY_HOSTS_STORE_COLUMN_FAVICON]     = GDK_TYPE_PIXBUF;

  gtk_list_store_set_column_types (GTK_LIST_STORE (self),
                                   EPHY_HOSTS_STORE_N_COLUMNS,
                                   types);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (self),
                                        EPHY_HOSTS_STORE_COLUMN_ADDRESS,
                                        GTK_SORT_ASCENDING);

#ifdef HAVE_WEBKIT2
  g_signal_connect (webkit_web_context_get_favicon_database (webkit_web_context_get_default ()),
                    "favicon-changed", G_CALLBACK (icon_changed_cb), self);
#else
  g_signal_connect (webkit_get_favicon_database (), "icon-loaded",
                    G_CALLBACK (icon_changed_cb), self);
#endif
}

EphyHostsStore *
ephy_hosts_store_new (void)
{
  return g_object_new (EPHY_TYPE_HOSTS_STORE,
                       NULL);
}

void
ephy_hosts_store_add_hosts (EphyHostsStore *store,
                            GList *hosts)
{
  EphyHistoryHost *host;
  GtkTreeIter treeiter;
  GtkTreePath *path;
  GList *iter;
  GdkPixbuf *favicon;
  IconLoadData *data;
  WebKitFaviconDatabase *database;

#ifdef HAVE_WEBKIT2
  database = webkit_web_context_get_favicon_database (webkit_web_context_get_default ());
#else
  database = webkit_get_favicon_database ();
#endif

  for (iter = hosts; iter != NULL; iter = iter->next) {
    host = (EphyHistoryHost *)iter->data;
#ifdef HAVE_WEBKIT2
    /* Flag favicon to NULL to reuse some code later on */
    favicon = NULL;
#else
    favicon = webkit_favicon_database_try_get_favicon_pixbuf (database, host->url,
                                                              FAVICON_SIZE, FAVICON_SIZE);
#endif

    gtk_list_store_insert_with_values (GTK_LIST_STORE (store),
                                       &treeiter, G_MAXINT,
                                       EPHY_HOSTS_STORE_COLUMN_ID, host->id,
                                       EPHY_HOSTS_STORE_COLUMN_TITLE, host->title,
                                       EPHY_HOSTS_STORE_COLUMN_ADDRESS, host->url,
                                       EPHY_HOSTS_STORE_COLUMN_VISIT_COUNT, host->visit_count,
#ifndef HAVE_WEBKIT2
                                       EPHY_HOSTS_STORE_COLUMN_FAVICON, favicon,
#endif
                                       -1);
    if (favicon)
      g_object_unref (favicon);
    else {
      data = g_slice_new (IconLoadData);
      data->model = GTK_LIST_STORE (g_object_ref (store));
      path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &treeiter);
      data->row_reference = gtk_tree_row_reference_new (GTK_TREE_MODEL (store), path);
      gtk_tree_path_free (path);

#ifdef HAVE_WEBKIT2
      webkit_favicon_database_get_favicon (database,
                                           host->url,
                                           NULL,
                                           async_update_favicon_icon,
                                           data);
#else
      webkit_favicon_database_get_favicon_pixbuf (database, host->url,
                                                  FAVICON_SIZE, FAVICON_SIZE, NULL,
                                                  async_update_favicon_icon, data);
#endif
    }
  }
}

void
ephy_hosts_store_add_host (EphyHostsStore *store, EphyHistoryHost *host)
{
  GList *hosts = NULL;
  hosts = g_list_append (hosts, host);
  ephy_hosts_store_add_hosts (store, hosts);
  g_list_free (hosts);
}

EphyHistoryHost *
ephy_hosts_store_get_host_from_path (EphyHostsStore *store,
                                     GtkTreePath *path)
{
  GtkTreeIter iter;

  EphyHistoryHost *host = ephy_history_host_new (NULL, NULL, 0, 1.0);

  gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
  gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
                      EPHY_HOSTS_STORE_COLUMN_ID, &host->id,
                      EPHY_HOSTS_STORE_COLUMN_TITLE, &host->title,
                      EPHY_HOSTS_STORE_COLUMN_ADDRESS, &host->url,
                      EPHY_HOSTS_STORE_COLUMN_VISIT_COUNT, &host->visit_count,
                      -1);
  return host;
}

void
ephy_hosts_store_clear (EphyHostsStore *store)
{
  gtk_list_store_clear (GTK_LIST_STORE (store));
  gtk_list_store_insert_with_values (GTK_LIST_STORE (store), NULL, 0,
                                     EPHY_HOSTS_STORE_COLUMN_ID, 0,
                                     EPHY_HOSTS_STORE_COLUMN_TITLE, _("All sites"),
                                     -1);
}
