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
  ".sha1",
  ".sha256",
  "CHECKSUM",
  NULL
};

static int checksum_type_lengths [] = {
  32,
  40,
  64,
  0, /* NO_CHECKSUM, has to be last */
}; /* SAME order as CheckcopyChecksumType */


static gchar * status_text [] = {
  N_("none"),
  N_("checksum found"),
  N_("verified"),
  N_("FAILED"),
  N_("copied"),
  /*
  CHECKCOPY_STATUS_NONE,
  CHECKCOPY_STATUS_VERIFIABLE,
  CHECKCOPY_STATUS_VERIFIED,
  CHECKCOPY_STATUS_VERIFICATION_FAILED,
  CHECKCOPY_STATUS_COPIED,
  */
}; // same order as CheckcopyFileStatus


/* internals */


/* public functions */
gssize
checkcopy_file_info_format_checksum (CheckcopyFileInfo * info, gchar ** line)
{
  gssize len;
  GString * string;

  g_assert (line != NULL);
  
  if (!info->relname || !info->checksum) {
    *line = NULL;
    return 0;
  }

  string = g_string_new ("");

  g_string_printf (string, "%s  %s\n", info->checksum, info->relname);

  *line = string->str;
  len = string->len;

  g_string_free (string, FALSE);

  return len; 
}

gint
checkcopy_file_info_cmp (CheckcopyFileInfo *infoa, CheckcopyFileInfo *infob)
{
  g_assert (infoa != NULL);
  g_assert (infob != NULL);

  return g_strcmp0 (infoa->relname, infob->relname);
}

void
checkcopy_file_info_free (CheckcopyFileInfo *info)
{
  g_free (info->relname);
  g_free (info->checksum);

  g_free (info);
}

gboolean
checkcopy_file_info_is_checksum_file (GFile *file)
{
  gchar * uri;
  int i;
  gchar * ext;
  gboolean r = FALSE;

  uri = g_file_get_uri (file);

  for (i=0; (ext = checksum_type_extensions[i]) != NULL && !r; i++) {

    if (g_str_has_suffix (uri, ext)) {
      r = TRUE;
    }
  }

  g_free (uri);

  return r;
}

CheckcopyChecksumType
checkcopy_file_info_get_checksum_type (gchar *checksum)
{
  int i;
  gsize l;

  l = strlen (checksum);

  for (i=0; checksum_type_lengths[i] != 0; i++) {
    if (l == checksum_type_lengths[i])
      return i;
  }

  return CHECKCOPY_NO_CHECKSUM;
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

const gchar * checkcopy_file_status_to_string (CheckcopyFileStatus status)
{
  return status_text[status];
}

const gchar *
checkcopy_file_info_status_text (CheckcopyFileInfo *info)
{
  g_assert (info != NULL);

  return checkcopy_file_status_to_string (info->status);
}
