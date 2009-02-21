/* $Id: global.h 50 2009-02-16 07:36:08Z squisher $ */
/*
 *  Copyright (c) 2008-2009 David Mohr <david@mcbf.net>
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

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <glib.h>
#include <stdarg.h>

#include "settings.h"

typedef struct {
  char buf[BUF_SIZE];
  int n;
  gchar *open_md5;
  gchar *write_hash;
  gboolean close;
  gboolean quit;
} workunit;

#define RING_BUFFER_TYPE workunit

#define XFCE_DISABLE_DEPRECATED

#endif /*  __GLOBAL_H__ */
