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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "checkcopy-file-handler.h"


/*- private prototypes -*/

static void checkcopy_file_handler_base_init (CheckcopyFileHandlerInterface * iface);

/*- globals -*/


/*****************/
/*- class setup -*/
/*****************/

GType
checkcopy_file_handler_get_type (void)
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (CheckcopyFileHandlerInterface),
      (GBaseInitFunc) checkcopy_file_handler_base_init,
      NULL,
      NULL,
      NULL,
      NULL,
      0,
      0,
      NULL,
      NULL,
    };

    type = g_type_register_static (G_TYPE_INTERFACE, "CheckcopyFileHandlerInterface", &our_info, 0);
    g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
  }

  return type;
}

static void
checkcopy_file_handler_base_init (CheckcopyFileHandlerInterface * iface)
{
}


/***************/
/*- internals -*/
/***************/

/*******************/
/*- public methods-*/
/*******************/


void
checkcopy_file_handler_process (CheckcopyFileHandler *fhandler, GFile *root, GFile *file, GFileInfo *info)
{
  CheckcopyFileHandlerInterface *iface = CHECKCOPY_FILE_HANDLER_GET_INTERFACE (fhandler);

  g_assert (root != NULL);
  g_assert (file != NULL);
  g_assert (info != NULL);

  iface->process (fhandler, root, file, info);
}

const gchar *
checkcopy_file_handler_get_attribute_list (CheckcopyFileHandler *fhandler)
{
  CheckcopyFileHandlerInterface *iface = CHECKCOPY_FILE_HANDLER_GET_INTERFACE (fhandler);

  return iface->get_attribute_list (fhandler);
}
