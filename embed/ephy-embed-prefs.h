/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/*  Copyright © 2008 Xan Lopez <xan@gnome.org>
 *  Copyright © 2009 Igalia S.L.
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

#ifndef __EPHY_EMBED_PREFS_H__
#define __EPHY_EMBED_PREFS_H__

#include "ephy-embed.h"

#ifdef HAVE_WEBKIT2
#include <webkit2/webkit2.h>
#else
#include <libsoup/soup.h>
#endif

#if !defined (__EPHY_EPIPHANY_H_INSIDE__) && !defined (EPIPHANY_COMPILATION)
#error "Only <epiphany/epiphany.h> can be included directly."
#endif

#define USER_STYLESHEET_FILENAME	"user-stylesheet.css"
#define FAVICON_SIZE 16

G_BEGIN_DECLS

void ephy_embed_prefs_init                              (void);
void ephy_embed_prefs_shutdown                          (void);

#ifdef HAVE_WEBKIT2
WebKitWebViewGroup *ephy_embed_prefs_get_web_view_group (void);
#else
void ephy_embed_prefs_add_embed                         (EphyEmbed           *embed);
#endif

#ifdef HAVE_WEBKIT2
void ephy_embed_prefs_set_cookie_accept_policy          (WebKitCookieManager *cookie_manager,
                                                         const char          *settings_policy);
#else
void ephy_embed_prefs_set_cookie_jar_policy             (SoupCookieJar       *jar,
                                                         const char          *gconf_policy);
#endif

G_END_DECLS

#endif /* __EPHY_EMBED_PREFS_H__ */
