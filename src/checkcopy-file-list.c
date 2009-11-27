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

static GObject * checkcopy_file_list_constructor (GType type, guint n_construct_params, GObjectConstructParam * construct_params);
static GOutputStream * get_checksum_stream (CheckcopyFileList * list, GFile * dest);

/*- globals -*/

#define MAX_CHECKSUM_FILE_RETRIES 10000

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
  GFile * checksum_file;

  /* statistics */

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

static GOutputStream * 
get_checksum_stream (CheckcopyFileList * list, GFile * dest)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (list);

  gchar * basename;
  gchar * checksum_name;
  GFileOutputStream * out;
  GError *error = NULL;
  GCancellable * cancel;
  GFile *checksum = NULL;
  gint i;
  gchar *ext = "CHECKSUM";

  cancel = checkcopy_get_cancellable ();
  basename = g_file_get_basename (dest);

  i = 0;
  do {
    DBG ("Try %d to generate a checksum file name", i);
    if (checksum) {
      g_object_unref (checksum);
      checksum = NULL;
    }

    if (error) {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
        /* There was an error, but it was not that the file already exists.
         * We should abort at this point. */

        g_error_free (error);
        error = NULL;
        break;
      } else {
        /* continue with next iteration */

        g_error_free (error);
        error = NULL;

        if (i > MAX_CHECKSUM_FILE_RETRIES)
          break;
      }
    }

    if (basename == NULL) {
      checksum_name = g_strdup (ext);
    } else {
      if (i==0)
        checksum_name = g_strconcat (basename, ".", ext, NULL);
      else
        checksum_name = g_strdup_printf ("%s-%d.%s", basename, i, ext);
    }

    checksum = g_file_resolve_relative_path (dest, checksum_name);
    i++;

  } while ((out = g_file_create (checksum, 0, cancel, &error)) == NULL);

  g_free (basename);

  if (out) {
    priv->checksum_file = checksum;

    return G_OUTPUT_STREAM (out);
  } else {
    g_object_unref (checksum);

    return NULL;
  }
}

/*******************/
/*- public methods-*/
/*******************/

gint
checksum_file_list_parse_checksum_file (CheckcopyFileList * list, GFile *root, GFile *file)
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
      CheckcopyChecksumType checksum_type;

      /* found a checksum, parse the line into
       * checksum and filename */

      n++;

      *c = '\0';
      c++;

      checksum = g_strdup (line);

      checksum_type = checkcopy_file_info_get_checksum_type (line);

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

        DBG ("Parsed checksum for %s", info->relname);

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
checkcopy_file_list_check_file (CheckcopyFileList * list, gchar *relname, const gchar *checksum, CheckcopyChecksumType checksum_type)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (list);
  CheckcopyFileInfo *info;

  info = g_hash_table_lookup (priv->files_hash, relname);

  if (info == NULL) {
    /* We have not seen a checksum for this file yet.
     * Record the checksum we just calculated */
    
    DBG ("%s had no checksum, recording %s", relname, checksum);

    info = g_new0 (CheckcopyFileInfo, 1);

    info->relname = g_strdup (relname);
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
      info->status = CHECKCOPY_STATUS_VERIFIED;
    }
  }

  return info->status;
}

gboolean
checkcopy_file_list_write_checksum (CheckcopyFileList * list, GFile * dest)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (list);

  GOutputStream * out;
  GCancellable * cancel;
  GError *error = NULL;

  cancel = checkcopy_get_cancellable ();

  out = get_checksum_stream (list, dest);

  if (out) {
    /* Write the file */
    GList * file_list;
    GList * curr_file;

    file_list = checkcopy_file_list_get_sorted_list (list);

    for (curr_file = file_list; curr_file != NULL; curr_file = g_list_next (curr_file)) {
      CheckcopyFileInfo * info = (CheckcopyFileInfo *) curr_file->data;
      gchar * line;
      gint n;
      gsize n_written;
      gboolean r;

      if (!(info->status==CHECKCOPY_STATUS_COPIED || info->status==CHECKCOPY_STATUS_VERIFICATION_FAILED)) {

        continue;
      }

      n = checkcopy_file_info_format_checksum (info, &line);

      r = g_output_stream_write_all (out, line, n, &n_written, cancel, &error);

      if (!r || n != n_written) {
        gchar * disp_name;

        disp_name = g_file_get_uri (priv->checksum_file);

        if (error) {
          g_critical ("While writing checksum file %s: %s", disp_name, error->message);

          g_error_free (error);
        } else {
          g_critical ("Only wrote %u of %d bytes of checksum file %s", n_written, n, disp_name);
        }
        g_free (disp_name);
      }

      g_free (line);
    }

    g_list_free (file_list);
  }

  return out != NULL;
}

GList *
checkcopy_file_list_get_sorted_list (CheckcopyFileList * list)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (list);

  GList * file_list;

  file_list = g_hash_table_get_values (priv->files_hash);

  file_list = g_list_sort (file_list, (GCompareFunc) checkcopy_file_info_cmp);

  return file_list;
}

CheckcopyFileList*
checkcopy_file_list_get_instance (void)
{
  return g_object_new (CHECKCOPY_TYPE_FILE_LIST, NULL);
}
