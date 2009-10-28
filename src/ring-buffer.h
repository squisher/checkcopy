/* $Id: ring-buffer.h 53 2009-02-16 09:39:17Z squisher $ */
/*
 *  Copyright (c) 2008 David Mohr <david@mcbf.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include <glib.h>
#include "global.h"

gboolean ring_buffer_init();

/* producer */
RING_BUFFER_TYPE * ring_buffer_put();
RING_BUFFER_TYPE * ring_buffer_get_producer_buffer ();
/* consumer */
RING_BUFFER_TYPE * ring_buffer_get(GError **error);

#ifdef STATS
gchar *ring_buffer_get_stats ();
#endif

void ring_buffer_free ();

#endif /* __RING_BUFFER_H__ */
