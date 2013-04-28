/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=2 sts=2 et: */
/*
 *  Copyright © 2013 Igalia S.L.
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
#include "ephy-test-utils.h"

#include "ephy-embed-shell.h"

#include <glib.h>

static guint web_view_ready_counter = 0;

guint
ephy_test_utils_get_web_view_ready_counter (void)
{
  return web_view_ready_counter;
}

void
ephy_test_utils_check_ephy_web_view_address (EphyWebView *view,
                                             const gchar *address)
{
  g_assert_cmpstr (ephy_web_view_get_address (view), ==, address);
}

void
ephy_test_utils_check_ephy_embed_address (EphyEmbed *embed,
                                          const gchar *address)
{
  ephy_test_utils_check_ephy_web_view_address (ephy_embed_get_web_view (embed), address);
}

static void
load_changed_cb (WebKitWebView *web_view,
#ifdef HAVE_WEBKIT2
                 WebKitLoadEvent status,
#else
                 GParamSpec *pspec,
#endif
                 GMainLoop *loop)
{
#ifndef HAVE_WEBKIT2
  WebKitLoadStatus status = webkit_web_view_get_load_status (web_view);
#endif

  if (status == WEBKIT_LOAD_COMMITTED) {
    web_view_ready_counter--;
    g_signal_handlers_disconnect_by_func (web_view, load_changed_cb, loop);

    if (web_view_ready_counter == 0)
      g_main_loop_quit (loop);
  }

}

static void
wait_until_load_is_committed (WebKitWebView *web_view, GMainLoop *loop)
{
#ifdef HAVE_WEBKIT2
  g_signal_connect (web_view, "load-changed", G_CALLBACK (load_changed_cb), loop);
#else
  g_signal_connect (web_view, "notify::load-status", G_CALLBACK (load_changed_cb), loop);
#endif
}

static void
web_view_created_cb (EphyEmbedShell *shell, EphyWebView *view, GMainLoop *loop)
{
  web_view_ready_counter++;
  wait_until_load_is_committed (WEBKIT_WEB_VIEW (view), loop);
}

GMainLoop*
ephy_test_utils_setup_ensure_web_views_are_loaded (void)
{
  GMainLoop *loop;

  web_view_ready_counter = 0;

  loop = g_main_loop_new (NULL, FALSE);
  g_signal_connect (ephy_embed_shell_get_default (), "web-view-created",
                    G_CALLBACK (web_view_created_cb), loop);

  return loop;
}

void
ephy_test_utils_ensure_web_views_are_loaded (GMainLoop *loop)
{
  if (web_view_ready_counter != 0)
    g_main_loop_run (loop);

  g_signal_handlers_disconnect_by_func (ephy_embed_shell_get_default (), G_CALLBACK (web_view_created_cb), loop);
  g_assert_cmpint (web_view_ready_counter, ==, 0);
  g_main_loop_unref (loop);
}

GMainLoop*
ephy_test_utils_setup_wait_until_load_is_committed (EphyWebView *view)
{
  GMainLoop *loop;

  web_view_ready_counter = 1;

  loop = g_main_loop_new (NULL, FALSE);
  wait_until_load_is_committed (WEBKIT_WEB_VIEW (view), loop);

  return loop;
}

void
ephy_test_utils_wait_until_load_is_committed (GMainLoop *loop)
{
  if (web_view_ready_counter != 0)
    g_main_loop_run (loop);

  g_assert_cmpint (web_view_ready_counter, ==, 0);
  g_main_loop_unref (loop);
}
