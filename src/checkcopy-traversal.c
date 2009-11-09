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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libxfce4util/libxfce4util.h>

#include "checkcopy-traversal.h"
#include "checkcopy-cancel.h"


/* internals */

gboolean checkcopy_traverse_file (CheckcopyFileHandler *fhandler, GFile *root, GFile *file, GError **error);


/* implementation */

gboolean
checkcopy_traverse_file (CheckcopyFileHandler *fhandler, GFile *root, GFile *file, GError **error)
{
  GFileInfo *fileinfo;
  const gchar *name;
  GCancellable *cancel;
  gchar *attribs;
  gboolean aborted = FALSE;

  attribs = g_strconcat (G_FILE_ATTRIBUTE_STANDARD_NAME "," G_FILE_ATTRIBUTE_STANDARD_TYPE "," G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME ",", 
                         checkcopy_file_handler_get_attribute_list (fhandler), NULL);

  //DBG ("Checking for '%s' attributes", attribs);

  cancel = checkcopy_get_cancellable();

  fileinfo = g_file_query_info (file, 
                                attribs,
                                G_FILE_QUERY_INFO_NONE, cancel, error);
  
  if (fileinfo != NULL) {

    name = g_file_info_get_display_name (fileinfo);

    if (g_file_info_get_file_type (fileinfo) == G_FILE_TYPE_DIRECTORY) {
      GFileEnumerator *iter;
      GFileInfo *child_info;

      //DBG ("%s is a directory", name);

      checkcopy_file_handler_process (fhandler, root, file, fileinfo);

      iter = g_file_enumerate_children (file,
                                        attribs,
                                        G_FILE_QUERY_INFO_NONE, NULL, error);

      if (iter != NULL) {
        while ((child_info = g_file_enumerator_next_file (iter, cancel, error)) != NULL) {
          GFile *child;
          gboolean ret;
          const gchar *child_name;

          child_name = g_file_info_get_name (child_info);

          child = g_file_resolve_relative_path (file, child_name);

          if (child != NULL) {
            ret = checkcopy_traverse_file (fhandler, root, child, error);
            if (!ret) {
              g_warning ("Error while traversing: %s", (*error)->message);
              g_error_free (*error);
            }
          }

          g_object_unref (child);
        } /* while */
      } /* if iter */

    } else {
#ifdef DEBUG
      gchar *relname;

      relname = g_file_get_relative_path (root, file);

      //DBG ("file %s", relname);

      g_free (relname);
#endif

      checkcopy_file_handler_process (fhandler, root, file, fileinfo);
    }

    aborted = TRUE;
  }

  g_free (attribs);

  return aborted;
}

void 
checkcopy_traverse (gchar **files, const gint count, CheckcopyFileHandler *fhandler)
{
  int i;
  GError *error = NULL;

  for (i=0; i<count; i++) {
    GFile *file;
    GFile *root;
   
    file = g_file_new_for_commandline_arg (files[i]);
    root = g_file_get_parent (file);

    checkcopy_traverse_file (fhandler, root, file, &error);

    g_object_unref (file);
    g_object_unref (root);
  }
}
