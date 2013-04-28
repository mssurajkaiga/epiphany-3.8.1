/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef EPHY_ADBLOCK_H
#define EPHY_ADBLOCK_H

#include "ephy-embed.h"

#include <glib-object.h>
#include <glib.h>

G_BEGIN_DECLS

#define EPHY_TYPE_ADBLOCK         (ephy_adblock_get_type ())
#define EPHY_ADBLOCK(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EPHY_TYPE_ADBLOCK, EphyAdBlock))
#define EPHY_ADBLOCK_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), EPHY_TYPE_ADBLOCK, EphyAdBlockClass))
#define EPHY_IS_ADBLOCK(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EPHY_TYPE_ADBLOCK))
#define EPHY_IS_ADBLOCK_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), EPHY_TYPE_ADBLOCK))
#define EPHY_ADBLOCK_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), EPHY_TYPE_ADBLOCK, EphyAdBlockClass))

typedef enum
{
  AD_URI_CHECK_TYPE_OTHER       = 1U,
  AD_URI_CHECK_TYPE_SCRIPT      = 2U, /* Indicates an executable script
                                         (such as JavaScript) */
  AD_URI_CHECK_TYPE_IMAGE       = 3U, /* Indicates an image (e.g., IMG
                                         elements) */
  AD_URI_CHECK_TYPE_STYLESHEET  = 4U, /* Indicates a stylesheet (e.g.,
                                         STYLE elements) */
  AD_URI_CHECK_TYPE_OBJECT      = 5U, /* Indicates a generic object
                                         (plugin-handled content
                                         typically falls under this
                                         category) */
  AD_URI_CHECK_TYPE_DOCUMENT    = 6U, /* Indicates a document at the
                                         top-level (i.e., in a
                                         browser) */
  AD_URI_CHECK_TYPE_SUBDOCUMENT = 7U, /* Indicates a document contained
                                         within another document (e.g.,
                                         IFRAMEs, FRAMES, and OBJECTs) */
  AD_URI_CHECK_TYPE_REFRESH     = 8U, /* Indicates a timed refresh */
  AD_URI_CHECK_TYPE_XBEL              =  9U, /* Indicates an XBL binding request,
                                                triggered either by -moz-binding CSS
                                                property or Document.addBinding method */
  AD_URI_CHECK_TYPE_PING              = 10U, /* Indicates a ping triggered by a click on
                                                <A PING="..."> element */
  AD_URI_CHECK_TYPE_XMLHTTPREQUEST    = 11U, /* Indicates a XMLHttpRequest */
  AD_URI_CHECK_TYPE_OBJECT_SUBREQUEST = 12U  /* Indicates a request by a plugin */
} AdUriCheckType;

typedef struct EphyAdBlock        EphyAdBlock;
typedef struct EphyAdBlockClass   EphyAdBlockClass;
typedef struct EphyAdBlockPrivate EphyAdBlockPrivate;

struct EphyAdBlockClass
{
  GObjectClass parent_class;
};

struct EphyAdBlock
{
  GObject parent_instance;

  /*< private >*/
  EphyAdBlockPrivate *priv;
};

GType    ephy_adblock_get_type    (void);

gboolean ephy_adblock_should_load (EphyAdBlock   *adblock,
                                   EphyEmbed     *embed,
                                   const char    *url,
                                   AdUriCheckType type);

G_END_DECLS

#endif
