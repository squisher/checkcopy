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

#ifndef __CHECKCOPY_FILE_HANDLER_BASE__
#define __CHECKCOPY_FILE_HANDLER_BASE__

#include <glib-object.h>
#include <gio/gio.h>
#include "progress-dialog.h"
#include "checkcopy-file-list.h"

G_BEGIN_DECLS

#define CHECKCOPY_TYPE_FILE_HANDLER_BASE checkcopy_file_handler_base_get_type()

#define CHECKCOPY_FILE_HANDLER_BASE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHECKCOPY_TYPE_FILE_HANDLER_BASE, CheckcopyFileHandlerBase))

#define CHECKCOPY_FILE_HANDLER_BASE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), CHECKCOPY_TYPE_FILE_HANDLER_BASE, CheckcopyFileHandlerBaseClass))

#define CHECKCOPY_IS_FILE_HANDLER_BASE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHECKCOPY_TYPE_FILE_HANDLER_BASE))

#define CHECKCOPY_IS_FILE_HANDLER_BASE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), CHECKCOPY_TYPE_FILE_HANDLER_BASE))

#define CHECKCOPY_FILE_HANDLER_BASE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHECKCOPY_TYPE_FILE_HANDLER_BASE, CheckcopyFileHandlerBaseClass))

typedef struct {
  GObject parent;

  /* protected */

  GFile *dest;
  ProgressDialog * progress_dialog;
  CheckcopyFileList * list;

  gboolean verify_only;
} CheckcopyFileHandlerBase;

typedef struct {
  GObjectClass parent_class;
} CheckcopyFileHandlerBaseClass;

GType checkcopy_file_handler_base_get_type (void);

CheckcopyFileHandlerBase* checkcopy_file_handler_base_new (void);

G_END_DECLS

#endif /* __CHECKCOPY_FILE_HANDLER_BASE__ */
