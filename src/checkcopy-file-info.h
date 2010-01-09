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
#include "checkcopy-checksum-type.h"

G_BEGIN_DECLS

/* Keep status_text and status_color in sync with this enum! */
typedef enum {
  /* initial state */
  CHECKCOPY_STATUS_NONE,
  /* checksum has been found */
  CHECKCOPY_STATUS_VERIFIABLE,
  /* file has been seen, but no checksum yet */
  CHECKCOPY_STATUS_FOUND,

  CHECKCOPY_STATUS_MARKER_PROCESSED,

  /* file is processed and checksum were found, it matched */
  CHECKCOPY_STATUS_VERIFIED,
  /* file is processed, but no checksum was found */
  CHECKCOPY_STATUS_COPIED,
  /* file is processed, but checksum did not match */
  CHECKCOPY_STATUS_VERIFICATION_FAILED,
  /* file processing failed (i.e. no read permissions) */
  CHECKCOPY_STATUS_FAILED,
  /* file had a checksum, but was not found */
  CHECKCOPY_STATUS_NOT_FOUND,

  CHECKCOPY_STATUS_LAST,
} CheckcopyFileStatus;

typedef struct {
  gchar *relname;
  gchar * checksum;
  CheckcopyChecksumType checksum_type;
  CheckcopyFileStatus status;
  gboolean seen;
  gboolean checksum_file;
} CheckcopyFileInfo;

#define VERIFY_SCHEME "verify"


void checkcopy_file_info_free (CheckcopyFileInfo *info);

gboolean checkcopy_file_info_is_checksum_file (CheckcopyFileInfo * info, GFile *file);

CheckcopyChecksumType checkcopy_file_info_get_checksum_type (gchar *checksum);
GChecksumType checkcopy_checksum_type_to_gio (CheckcopyChecksumType type);

gssize checkcopy_file_info_format_checksum (CheckcopyFileInfo * info, gchar ** line);
gint checkcopy_file_info_cmp (CheckcopyFileInfo *infoa, CheckcopyFileInfo *infob);
const gchar * checkcopy_file_status_to_string (CheckcopyFileStatus status);
const gchar * checkcopy_file_info_status_text (CheckcopyFileInfo *info);
const gchar * checkcopy_file_info_status_color (CheckcopyFileStatus status);

G_END_DECLS

#endif /* _HAVE_CHECKCOPY_FILE_INFO */
