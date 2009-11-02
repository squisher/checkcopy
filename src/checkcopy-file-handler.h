/*
 *  Copyright (C) 2009 David Mohr <david@mcbf.net>
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
 *  
 */

#ifndef __CHECKCOPY_FILE_HANDLER__
#define __CHECKCOPY_FILE_HANDLER__

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define CHECKCOPY_TYPE_FILE_HANDLER checkcopy_file_handler_get_type()

#define CHECKCOPY_FILE_HANDLER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHECKCOPY_TYPE_FILE_HANDLER, CheckcopyFileHandler))

#define CHECKCOPY_IS_FILE_HANDLER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHECKCOPY_TYPE_FILE_HANDLER))

#define CHECKCOPY_FILE_HANDLER_GET_INTERFACE(obj) \
  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), CHECKCOPY_TYPE_FILE_HANDLER, CheckcopyFileHandlerInterface))

typedef struct {} CheckcopyFileHandler;

typedef struct {
  GTypeInterface parent;

  void (*process) (CheckcopyFileHandler *fhandler, GFile *root, GFile *file, GFileInfo *info);
  const gchar * (*get_attribute_list) (CheckcopyFileHandler *fhandler);
} CheckcopyFileHandlerInterface;

GType checkcopy_file_handler_get_type (void);

void checkcopy_file_handler_process (CheckcopyFileHandler *fhandler, GFile *root, GFile *file, GFileInfo *info);
const gchar * checkcopy_file_handler_get_attribute_list (CheckcopyFileHandler *fhandler);


G_END_DECLS

#endif /* __CHECKCOPY_FILE_HANDLER__ */
