/*
 * Copyright (c) 2009     David Mohr <david@mcbf.net>
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
#ifndef _HAVE_CHECKCOPY_WORKER
#define _HAVE_CHECKCOPY_WORKER

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct {
  GAsyncQueue * queue;
  GFile * dest;
} CheckcopyWorkerParams;

void checkcopy_worker (CheckcopyWorkerParams * params) G_GNUC_NORETURN;

G_END_DECLS

#endif /* _HAVE_CHECKCOPY_WORKER */
