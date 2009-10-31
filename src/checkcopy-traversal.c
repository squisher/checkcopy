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

#include "checkcopy-traversal.h"

/* internals */

gboolean checkcopy_traverse_file (GFile *file, GError **error);


/* implementation */

gboolean
checkcopy_traverse_file (GFile *file, GError **error)
{
  GFileInfo *fileinfo;
  const gchar *name;

  fileinfo = g_file_query_info (file, 
                                G_FILE_ATTRIBUTE_STANDARD_TYPE "," G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                                G_FILE_QUERY_INFO_NONE, NULL, error);
  
  if (fileinfo == NULL)
    return FALSE;

  name = g_file_info_get_display_name (fileinfo);

  if (g_file_info_get_file_type (fileinfo) == G_FILE_TYPE_DIRECTORY) {
    g_message ("%s is a directory\n", name);
  } else {
    g_message ("%s is a file\n", name);
  }

  return TRUE;
}

void 
checkcopy_traverse (gchar **files, const gint count)
{
  int i;
  GError *error = NULL;

  for (i=0; i<count; i++) {
    GFile *file = g_file_new_for_commandline_arg (files[i]);

    checkcopy_traverse_file (file, &error);
  }
}
