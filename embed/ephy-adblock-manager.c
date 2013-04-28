/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 *  Copyright © 2003 Marco Pesenti Gritti
 *  Copyright © 2003 Christian Persch
 *  Copyright © 2005 Jean-François Rameau
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
#include "ephy-adblock-manager.h"

#include "ephy-adblock.h"
#include "ephy-debug.h"

struct _EphyAdBlockManagerPrivate
{
  EphyAdBlock *blocker;
};

G_DEFINE_TYPE (EphyAdBlockManager, ephy_adblock_manager, G_TYPE_OBJECT);

#define EPHY_ADBLOCK_MANAGER_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), EPHY_TYPE_ADBLOCK_MANAGER, EphyAdBlockManagerPrivate))

/**
 * ephy_adblock_manager_set_blocker:
 * @shell: a #EphyAdBlockManager
 * @blocker: the new blocker or NULL
 *
 * Set a new ad blocker. If #blocker is %NULL,
 * ad blocking is toggled off.
 *
 **/
void
ephy_adblock_manager_set_blocker (EphyAdBlockManager *self,
                                  EphyAdBlock *blocker)
{
  g_return_if_fail (EPHY_IS_ADBLOCK_MANAGER (self));
  g_return_if_fail (EPHY_IS_ADBLOCK (blocker));

  self->priv->blocker = blocker;
}

/**
 * ephy_adblock_manager_should_load:
 * @shell: a #EphyAdBlockManager
 * @url: the target url to be loaded or not
 * @AdUriCheckType: what check to be applied (image, script, ...)
 *
 * Check if an url is to be loaded or not 
 *
 * Return value: TRUE if the url is to be loaded
 **/
gboolean
ephy_adblock_manager_should_load (EphyAdBlockManager *self,
                                  EphyEmbed *embed,
                                  const char *url,
                                  AdUriCheckType check_type)
{
  g_return_val_if_fail (EPHY_IS_ADBLOCK_MANAGER (self), TRUE);
  g_return_val_if_fail (EPHY_IS_EMBED (embed), TRUE);
  g_return_val_if_fail (url, TRUE);

  if (self->priv->blocker != NULL)
    return ephy_adblock_should_load (self->priv->blocker, 
                                     embed,
                                     url,
                                     check_type);

  /* Default: let's process any url. */
  return TRUE;
}

static void
ephy_adblock_manager_init (EphyAdBlockManager *self)
{
  LOG ("ephy_adblock_manager_init");

  self->priv = EPHY_ADBLOCK_MANAGER_GET_PRIVATE(self);
}

static void
ephy_adblock_manager_class_init (EphyAdBlockManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_signal_new ("rules_changed",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (EphyAdBlockManagerClass, rules_changed),
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0,
                0);

  g_type_class_add_private (object_class, sizeof (EphyAdBlockManagerPrivate));
}

/**
 * ephy_adblock_manager_has_blocker:
 * @shell: a #EphyAdBlockManager
 *
 * Check if Epiphany has currently an active blocker
 *
 * Return value: TRUE if an active blocker is running
 **/
gboolean
ephy_adblock_manager_has_blocker (EphyAdBlockManager *self)
{
  g_return_val_if_fail (EPHY_IS_ADBLOCK_MANAGER (self), FALSE);

  return self->priv->blocker != NULL;
} 
