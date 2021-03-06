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

#ifndef __CHECKCOPY_FILE_LIST__
#define __CHECKCOPY_FILE_LIST__

#include <glib-object.h>
#include <gio/gio.h>

#include "checkcopy-file-info.h"

G_BEGIN_DECLS

#define CHECKCOPY_TYPE_FILE_LIST checkcopy_file_list_get_type()

#define CHECKCOPY_FILE_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHECKCOPY_TYPE_FILE_LIST, CheckcopyFileList))

#define CHECKCOPY_FILE_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), CHECKCOPY_TYPE_FILE_LIST, CheckcopyFileListClass))

#define CHECKCOPY_IS_FILE_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHECKCOPY_TYPE_FILE_LIST))

#define CHECKCOPY_IS_FILE_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), CHECKCOPY_TYPE_FILE_LIST))

#define CHECKCOPY_FILE_LIST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHECKCOPY_TYPE_FILE_LIST, CheckcopyFileListClass))

typedef struct {
  GObject parent;
} CheckcopyFileList;

typedef struct {
  GObjectClass parent_class;
} CheckcopyFileListClass;

typedef enum {
  CHECKCOPY_FILE_LIST_COUNT_VERIFIED,
  CHECKCOPY_FILE_LIST_COUNT_COPIED,
  CHECKCOPY_FILE_LIST_COUNT_FAILED,
  CHECKCOPY_FILE_LIST_COUNT_NOT_FOUND,
  CHECKCOPY_FILE_LIST_COUNT_LAST,
} CheckcopyFileListCount;

typedef struct {
  guint count[CHECKCOPY_FILE_LIST_COUNT_LAST];
} CheckcopyFileListStats;

GType checkcopy_file_list_get_type (void);

gint checksum_file_list_parse_checksum_file (CheckcopyFileList * list, GFile *root, GFile *file);
gboolean checkcopy_file_list_write_checksum (CheckcopyFileList * list, GFile * dest);

CheckcopyFileInfo * checkcopy_file_list_grab_info (CheckcopyFileList * list, gchar *relname);
CheckcopyFileStatus checkcopy_file_list_get_status (CheckcopyFileList * list, gchar *relname);
CheckcopyChecksumType checkcopy_file_list_get_file_type (CheckcopyFileList * list, gchar *relname);
CheckcopyFileStatus checkcopy_file_list_check_file (CheckcopyFileList * list, gchar *relname, const gchar *checksum, CheckcopyChecksumType checksum_type);
void checkcopy_file_list_mark_failed (CheckcopyFileList * list, gchar * relname);
gboolean checkcopy_file_list_transition (CheckcopyFileList * list,
                                                CheckcopyFileInfo * info, CheckcopyFileStatus new_status);

GList * checkcopy_file_list_get_display_list (CheckcopyFileList * list);
CheckcopyFileListStats * checkcopy_file_list_get_stats (CheckcopyFileList * list);
CheckcopyFileStatus checkcopy_file_list_status_to_info(CheckcopyFileListCount i);
void checkcopy_file_list_sweep (CheckcopyFileList * list);

CheckcopyFileList* checkcopy_file_list_get_instance (void);

G_END_DECLS

#endif /* __CHECKCOPY_FILE_LIST__ */
