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
#ifndef _HAVE_CHECKCOPY_FILE_INFO
#define _HAVE_CHECKCOPY_FILE_INFO

#include <gio/gio.h>

G_BEGIN_DECLS

/* Keep status_text and status_color in sync with this enum! */
typedef enum {
  CHECKCOPY_STATUS_NONE,
  CHECKCOPY_STATUS_VERIFIABLE,
  CHECKCOPY_STATUS_MARKER_PROCESSED,
  CHECKCOPY_STATUS_VERIFIED,
  CHECKCOPY_STATUS_COPIED,
  CHECKCOPY_STATUS_VERIFICATION_FAILED,
  CHECKCOPY_STATUS_FAILED,
  CHECKCOPY_STATUS_LAST,
} CheckcopyFileStatus;

typedef enum {
  CHECKCOPY_MD5,
  CHECKCOPY_SHA1,
  CHECKCOPY_SHA256,
  CHECKCOPY_NO_CHECKSUM, /* Has to be the last, so the order can
                            match checksum_type_extensions */
} CheckcopyChecksumType;

typedef struct {
  gchar *relname;
  gchar * checksum;
  CheckcopyChecksumType checksum_type;
  CheckcopyFileStatus status;
} CheckcopyFileInfo;

#define VERIFY_SCHEME "verify"


void checkcopy_file_info_free (CheckcopyFileInfo *info);

gboolean checkcopy_file_info_is_checksum_file (GFile *file);

CheckcopyChecksumType checkcopy_file_info_get_checksum_type (gchar *checksum);
GChecksumType checkcopy_checksum_type_to_gio (CheckcopyChecksumType type);

gssize checkcopy_file_info_format_checksum (CheckcopyFileInfo * info, gchar ** line);
gint checkcopy_file_info_cmp (CheckcopyFileInfo *infoa, CheckcopyFileInfo *infob);
const gchar * checkcopy_file_status_to_string (CheckcopyFileStatus status);
const gchar * checkcopy_file_info_status_text (CheckcopyFileInfo *info);
const gchar * checkcopy_file_info_status_color (CheckcopyFileStatus status);

G_END_DECLS

#endif /* _HAVE_CHECKCOPY_FILE_INFO */
