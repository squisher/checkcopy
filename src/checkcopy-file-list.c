/* checkcopy-file-list.c */
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

#include <gio/gio.h>

#include <libxfce4util/libxfce4util.h>

#include "checkcopy-file-list.h"
#include "checkcopy-cancel.h"

/*- private prototypes -*/

static GObject *
checkcopy_file_list_constructor (GType type, guint n_construct_params, GObjectConstructParam * construct_params);

/*- globals -*/

enum {
  PROP_0,
};

static CheckcopyFileList * singleton = NULL;


/*****************/
/*- class setup -*/
/*****************/

static GObjectClass * parent_class = NULL;

G_DEFINE_TYPE (CheckcopyFileList, checkcopy_file_list, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHECKCOPY_TYPE_FILE_LIST, CheckcopyFileListPrivate))

typedef struct _CheckcopyFileListPrivate CheckcopyFileListPrivate;


struct _CheckcopyFileListPrivate {
  GHashTable *files_hash;
};

static void
checkcopy_file_list_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  //CheckcopyFileListPrivate *priv = GET_PRIVATE (CHECKCOPY_FILE_LIST (object));

  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
checkcopy_file_list_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  //CheckcopyFileListPrivate *priv = GET_PRIVATE (CHECKCOPY_FILE_LIST (object));

  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
checkcopy_file_list_finalize (GObject *object)
{
  G_OBJECT_CLASS (checkcopy_file_list_parent_class)->finalize (object);
}

static void
checkcopy_file_list_class_init (CheckcopyFileListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CheckcopyFileListPrivate));
  parent_class = g_type_class_peek_parent (klass);

  object_class->get_property = checkcopy_file_list_get_property;
  object_class->set_property = checkcopy_file_list_set_property;
  object_class->finalize = checkcopy_file_list_finalize;
  object_class->constructor = checkcopy_file_list_constructor;
}

static void
checkcopy_file_list_init (CheckcopyFileList *self)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (self);

  priv->files_hash = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify) checkcopy_file_info_free);
}

static GObject *
checkcopy_file_list_constructor (GType type, guint n_construct_params, GObjectConstructParam * construct_params)
{
  GObject * obj;

  if (!singleton) {
    obj = G_OBJECT_CLASS (parent_class)->constructor (type, n_construct_params, construct_params);
    singleton = CHECKCOPY_FILE_LIST (obj);
  } else {
    obj = g_object_ref (G_OBJECT (singleton));
  }

  return obj;
}

/***************/
/*- internals -*/
/***************/

/*******************/
/*- public methods-*/
/*******************/

gint
checksum_file_list_parse_checksum_file (CheckcopyFileList * list, GFile *root, GFile *file, CheckcopyChecksumType checksum_type)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (list);
  GDataInputStream * in;
  gchar * line;
  gsize length;
  GCancellable *cancel;
  GError * error = NULL;
  GFile * parent;
  gchar * prefix;
  gint n = 0;

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

    if (*line == ';' || *line == '#') {
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

      n++;

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


      info = g_hash_table_lookup (priv->files_hash, filename);

      if (info == NULL) {
        info = g_new0 (CheckcopyFileInfo, 1);

        info->relname = filename;
        info->checksum = checksum;
        info->checksum_type = checksum_type;
        info->status = CHECKCOPY_STATUS_VERIFIABLE;

        g_hash_table_insert (priv->files_hash, info->relname, info);
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

  return n;
}

CheckcopyChecksumType
checkcopy_file_list_get_file_type (CheckcopyFileList * list, gchar *relname)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (list);
  CheckcopyFileInfo *info;

  info = g_hash_table_lookup (priv->files_hash, relname);

  if (info == NULL) {
    return CHECKCOPY_NO_CHECKSUM;
  } else {
    return info->checksum_type;
  }
}

CheckcopyFileStatus
checkcopy_file_list_check_file (CheckcopyFileList * list,gchar *relname, const gchar *checksum, CheckcopyChecksumType checksum_type)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (list);
  CheckcopyFileInfo *info;

  info = g_hash_table_lookup (priv->files_hash, relname);

  if (info == NULL) {
    /* We have not seen a checksum for this file yet.
     * Record the checksum we just calculated */
    
    DBG ("%s had no checksum, recording %s", relname, checksum);

    info = g_new0 (CheckcopyFileInfo, 1);

    info->relname = relname;
    info->checksum = g_strdup (checksum);
    info->checksum_type = checksum_type;
    info->status = CHECKCOPY_STATUS_COPIED;

    g_hash_table_insert (priv->files_hash, info->relname, info);
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

CheckcopyFileList*
checkcopy_file_list_get_instance (void)
{
  return g_object_new (CHECKCOPY_TYPE_FILE_LIST, NULL);
}
