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
#ifndef _HAVE_CHECKCOPY_TRAVERSAL
#define _HAVE_CHECKCOPY_TRAVERSAL

#include <gio/gio.h>

#include "checkcopy-file-handler.h"

G_BEGIN_DECLS

void checkcopy_traverse_args (gchar **files, const gint count, CheckcopyFileHandler *fhandler);
void checkcopy_traverse (GFile *file, CheckcopyFileHandler *fhandler);

G_END_DECLS

#endif /* _HAVE_CHECKCOPY_TRAVERSAL */
