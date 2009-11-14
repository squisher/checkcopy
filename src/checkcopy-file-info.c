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

#include "checkcopy-file-info.h"
#include "checkcopy-cancel.h"


/* globals */

static gchar * checksum_type_extensions [] = {
  ".md5",
  ".sha",
  ".sha256",
  NULL
}; // same order as CheckscopyChecksumType

static GHashTable * file_info_table = NULL;


/* internals */

static void
file_info_content_free (CheckcopyFileInfo *info)
{
  g_free (info->relname);
  g_free (info->checksum);

  g_free (info);
}


/* public functions */

CheckcopyChecksumType
checkcopy_file_info_get_checksum_type (GFile *file)
{
  gchar * uri;
  int i;
  gchar * ext;

  uri = g_file_get_uri (file);

  for (i=0; (ext = checksum_type_extensions[i]) != NULL; i++) {

    if (g_str_has_suffix (uri, ext)) {
      return i;
    }
  }

  g_free (uri);

  return CHECKCOPY_NO_CHECKSUM;
}


void
checksum_file_info_parse_checksum_file (GFile *root, GFile *file, CheckcopyChecksumType checksum_type)
{
  GDataInputStream * in;
  gchar * line;
  gsize length;
  GCancellable *cancel;
  GError * error = NULL;
  GFile * parent;
  gchar * prefix;

  cancel = checkcopy_get_cancellable ();

  parent = g_file_get_parent (file);
  prefix = g_file_get_relative_path (root, parent);

  g_assert (prefix != NULL);

  in = g_data_input_stream_new (G_INPUT_STREAM (g_file_read (file, cancel, &error)));

  while ((line = g_data_input_stream_read_line (in,
                                                &length,
                                                cancel,
                                                &error)) != NULL)
  {
    gchar *c;

    if (*line == ';') {
      /* skip comments */
      continue;
    }

    for (c = line; *c != ' ' && *c != '\0'; c++);

    if (c != line && *c != '\0') {
      gchar * checksum = NULL;
      gchar * filename = NULL;
      CheckcopyFileInfo * info;

      /* found a checksum, parse the line into
       * checksum and filename */

      *c = '\0';
      c++;

      checksum = g_strdup (line);

      /* skip spaces */
      while (*c == ' ' && *c != '\0') c++;

      /* some programs mark filenames with a star */
      if (*c == '*')
        c++;

      /* rest of the line is the file name */

      if (*prefix != '\0')
        filename = g_strconcat (prefix, "/", c, NULL);
      else
        filename = g_strdup (c);


      info = g_hash_table_lookup (file_info_table, filename);

      if (info == NULL) {
        info = g_new0 (CheckcopyFileInfo, 1);

        info->relname = filename;
        info->checksum = checksum;
        info->checksum_type = checksum_type;
        info->status = CHECKCOPY_STATUS_VERIFIABLE;

        g_hash_table_insert (file_info_table, info->relname, info);
      } else {
        /* We already have a checksum for this file. This is a bit odd.
         * Check if it matches with the previous one, otherwise note
         * an error */

        if (!g_str_equal (info->checksum, checksum)) {
          /* TODO: add to error list */

          g_warning ("Previous checksum for %s was %s, now encountered %s", filename, info->checksum, checksum);
        }
      }
    }

    g_free (line);
  }

  g_object_unref (parent);
  g_free (prefix);

  g_input_stream_close (G_INPUT_STREAM (in), cancel, &error);
}

CheckcopyFileStatus
checkcopy_file_info_check_file (gchar *relname, const gchar *checksum, CheckcopyChecksumType checksum_type)
{
  CheckcopyFileInfo *info;

  info = g_hash_table_lookup (file_info_table, relname);

  if (info == NULL) {
    /* We have not seen a checksum for this file yet.
     * Record the checksum we just calculated */
    
    DBG ("%s had no checksum, recording %s", relname, checksum);

    info = g_new0 (CheckcopyFileInfo, 1);

    info->relname = relname;
    info->checksum = g_strdup (checksum);
    info->checksum_type = checksum_type;
    info->status = CHECKCOPY_STATUS_COPIED;

    g_hash_table_insert (file_info_table, info->relname, info);
  } else {
    /* We have a checksum, verify it and record the result */

    if (!g_str_equal (info->checksum, checksum)) {
      /* Verification failed */
      // TODO: update the error list

      info->status = CHECKCOPY_STATUS_VERIFICATION_FAILED;

      g_warning ("%s was supposed to have checksum %s, but it had %s", relname, info->checksum, checksum);
    } else {
      /* Verification passed */

      DBG ("%s matched checksum %s", relname, checksum);
    }
  }

  return info->status;
}

CheckcopyChecksumType
checkcopy_file_info_get_type (gchar *relname)
{
  CheckcopyFileInfo *info;

  info = g_hash_table_lookup (file_info_table, relname);

  if (info == NULL) {
    return CHECKCOPY_NO_CHECKSUM;
  } else {
    return info->checksum_type;
  }
}

GChecksumType
checkcopy_checksum_type_to_gio (CheckcopyChecksumType type)
{
  switch (type) {
    case CHECKCOPY_MD5:
      return G_CHECKSUM_MD5;
    case CHECKCOPY_SHA1:
      return G_CHECKSUM_SHA1;
    case CHECKCOPY_SHA256:
      return G_CHECKSUM_SHA256;
    case CHECKCOPY_NO_CHECKSUM:
      g_error ("NO_CHECKSUM cannot be translated to GIO");
      return -1;
  }

  g_error ("No CheckcopyChecksumType matched, error");
  return -1;
}

void
checkcopy_file_info_init (void)
{
  g_assert (file_info_table == NULL);

  file_info_table = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify) file_info_content_free);
}

void
checkcopy_file_info_free (void)
{
  g_hash_table_destroy (file_info_table);
}
