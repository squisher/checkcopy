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
#include "error.h"
#include "checkcopy-worker.h"
#include "ompa-list.h"

/*- private prototypes -*/

static GObject * checkcopy_file_list_constructor (GType type, guint n_construct_params, GObjectConstructParam * construct_params);
static GOutputStream * get_checksum_stream (CheckcopyFileList * list, GFile * dest);
static GList * checkcopy_file_list_get_sorted_list (CheckcopyFileList * list);
static void mark_not_found (gpointer key, gpointer value, gpointer data);
static gboolean verify_list_filter (gconstpointer data);
static gboolean keep_checksum_stats (CheckcopyFileStatus status, const CheckcopyFileInfo * info);

/*- globals -*/

#define MAX_CHECKSUM_FILE_RETRIES 10000

enum {
  PROP_0,
  PROP_VERIFY_ONLY,
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

  gboolean verify_only;

  GMutex * stats_mutex;
  CheckcopyFileListStats stats;
};

static void
checkcopy_file_list_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (CHECKCOPY_FILE_LIST (object));

  switch (property_id) {
    case PROP_VERIFY_ONLY:
      g_value_set_boolean (value, priv->verify_only);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
checkcopy_file_list_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (CHECKCOPY_FILE_LIST (object));

  switch (property_id) {
    case PROP_VERIFY_ONLY:
      priv->verify_only = g_value_get_boolean (value);
      break;
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

  g_object_class_install_property (object_class, PROP_VERIFY_ONLY,
           g_param_spec_boolean ("verify-only", _("Only verify"), _("Only verify"), FALSE, G_PARAM_READWRITE));
}

static void
checkcopy_file_list_init (CheckcopyFileList *self)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (self);

  priv->files_hash = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify) checkcopy_file_info_free);

  priv->stats_mutex = g_mutex_new ();
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
  GFileOutputStream * out = NULL;
  GError *error = NULL;
  GCancellable * cancel;
  GFile *checksum = NULL;
  gint i;
  gchar *ext = "CHECKSUM";

  cancel = checkcopy_get_cancellable ();

  if (priv->checksum_file) {
    /* We already have a file. Just append to it. */
    out = g_file_append_to (priv->checksum_file, 0, cancel, &error);

    if (!out || error) {
      thread_show_gerror (dest, error);
      g_error_free (error);
      error = NULL;

      return NULL;
    } else {
      return G_OUTPUT_STREAM (out);
    }
  }


  /* We do not have a file yet, search for one */

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

        thread_show_error (_("Failed to create checksum file: %s"), error->message);
        g_error_free (error);
        error = NULL;
        break;
      } else {
        /* continue with next iteration */

        g_error_free (error);
        error = NULL;

        if (i > MAX_CHECKSUM_FILE_RETRIES) {
          thread_show_error (_("Maximum number of retries reached.\nCould not create a checksum file."));
          break;
        }
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

    if (g_cancellable_set_error_if_cancelled (cancel, &error)) {
      break;
    }

  } while ((out = g_file_create (checksum, 0, cancel, &error)) == NULL);

  g_free (basename);

  if (out) {
    priv->checksum_file = checksum;

    return G_OUTPUT_STREAM (out);
  } else {
    if (checksum)
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

  in = g_data_input_stream_new (G_INPUT_STREAM (g_file_read (file, cancel, &error)));

  while ((line = g_data_input_stream_read_line (in,
                                                &length,
                                                cancel,
                                                &error)) != NULL)
  {
    gchar *c;

    if (*line == ';' || *line == '#') {
      /* skip comment lines */
      continue;
    }

    /* find the end of the first column */
    for (c = line; *c != ' ' && *c != '\0'; c++);

    /* make sure we found some chars and we don't just have one column */
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

      if (prefix != NULL && *prefix != '\0')
        filename = g_strconcat (prefix, G_DIR_SEPARATOR_S, c, NULL);
      else
        filename = g_strdup (c);


      info = checkcopy_file_list_grab_info (list, filename);

      if (info->status == CHECKCOPY_STATUS_NONE) {
        info->checksum = checksum;
        info->checksum_type = checksum_type;
        checkcopy_file_list_transition (list, info, CHECKCOPY_STATUS_VERIFIABLE);

        DBG ("Parsed checksum for %s", info->relname);

        if (priv->verify_only) {
          checkcopy_worker_add_file (g_file_resolve_relative_path (root, filename));
        }
      } else {
        /* We saw the file before the checksum.
         *
         * Verify it now.
         */

        DBG ("%s was copied already, verifying it immediately", filename);

        if (!g_str_equal (info->checksum, checksum)) {
          /* Verification failed. We want to display the checksum
           * the file is supposed to have in the gui, so switch
           * the two variables.
           */
          gchar * ts;

          ts = info->checksum;
          info->checksum = checksum;
          checksum = ts;

          /* TODO: display the checksum which the file actually has */
          g_free (checksum);

          checkcopy_file_list_transition (list, info, CHECKCOPY_STATUS_VERIFICATION_FAILED);
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

CheckcopyFileInfo *
checkcopy_file_list_grab_info (CheckcopyFileList * list, gchar *relname)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (list);
  CheckcopyFileInfo *info;

  info = g_hash_table_lookup (priv->files_hash, relname);

  if (info == NULL) {
    info = g_new0 (CheckcopyFileInfo, 1);
    info->relname = g_strdup (relname);

    g_hash_table_insert (priv->files_hash, info->relname, info);
  }

  return info;
}

CheckcopyFileStatus
checkcopy_file_list_get_status (CheckcopyFileList * list, gchar *relname)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (list);
  CheckcopyFileInfo *info;

  info = g_hash_table_lookup (priv->files_hash, relname);

  /* at this point the relname should be known, since the planner has seen it */
  if (info == NULL)
    return CHECKCOPY_STATUS_NONE;

  return info->status;
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

  g_assert (info != NULL);

  if (info->status == CHECKCOPY_STATUS_FOUND) {
    /* We have not seen a checksum for this file yet.
     * Record the checksum we just calculated */

    DBG ("%s had no checksum, recording %s", relname, checksum);

    info->checksum = g_strdup (checksum);
    info->checksum_type = checksum_type;
    checkcopy_file_list_transition (list, info, CHECKCOPY_STATUS_COPIED);

  } else {
    /* We have a checksum, verify it and record the result */

    if (!g_str_equal (info->checksum, checksum)) {
      /* Verification failed */
      // TODO: update the error list

      checkcopy_file_list_transition (list, info, CHECKCOPY_STATUS_VERIFICATION_FAILED);

      g_warning ("%s was supposed to have checksum %s, but it had %s", relname, info->checksum, checksum);
    } else {
      /* Verification passed */

      DBG ("%s matched checksum %s", relname, checksum);
      checkcopy_file_list_transition (list, info, CHECKCOPY_STATUS_VERIFIED);
    }
  }

  return info->status;
}

void
checkcopy_file_list_mark_failed (CheckcopyFileList * list, gchar * relname)
{
//  CheckcopyFileListPrivate *priv = GET_PRIVATE (list);
  CheckcopyFileInfo *info;

  info = checkcopy_file_list_grab_info (list, relname);

  checkcopy_file_list_transition (list, info, CHECKCOPY_STATUS_FAILED);
}

gboolean
checkcopy_file_list_write_checksum (CheckcopyFileList * list, GFile * dest)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (list);

  GOutputStream * out;
  GCancellable * cancel;
  GError *error = NULL;
  gboolean aborted;

  cancel = checkcopy_get_cancellable ();

  if (priv->verify_only) {
    g_message ("Not writing checksum file because we are only verifying");
    return TRUE;
  }

  /* If we got cancelled, we still want to try this operation.
   * So remember the state of the cancellable, and continue
   */
  if ((aborted = g_cancellable_is_cancelled (cancel)) == TRUE)
    g_cancellable_reset (cancel);

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
      gboolean r = FALSE;

      if (!(info->status==CHECKCOPY_STATUS_COPIED || info->status==CHECKCOPY_STATUS_VERIFICATION_FAILED)) {

        continue;
      }

      n = checkcopy_file_info_format_checksum (info, &line);

      if (!g_cancellable_set_error_if_cancelled (cancel, &error))
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

      if (error) {
        thread_show_gerror (dest, error);
        g_error_free (error);
        error = NULL;
        g_free (line);
        break;
      }

      g_free (line);
    } /* for */


    if (!g_cancellable_set_error_if_cancelled (cancel, &error))
      g_output_stream_close (out, cancel, &error);

    if (error) {
      thread_show_gerror (dest, error);
      g_error_free (error);
      error = NULL;
    }

    g_list_free (file_list);
  }

  if (aborted && !g_cancellable_is_cancelled (cancel)) {
    g_cancellable_cancel (cancel);
  }

  return out != NULL;
}

static GList *
checkcopy_file_list_get_sorted_list (CheckcopyFileList * list)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (list);

  GList * file_list;

  file_list = g_hash_table_get_values (priv->files_hash);

  file_list = g_list_copy (file_list);

  file_list = g_list_sort (file_list, (GCompareFunc) checkcopy_file_info_cmp);

  return file_list;
}

static gboolean
keep_checksum_stats (CheckcopyFileStatus status, const CheckcopyFileInfo * info)
{
  return (status != CHECKCOPY_STATUS_COPIED) || !info->checksum_file;
}

static gboolean
verify_list_filter (gconstpointer data)
{
  const CheckcopyFileInfo * info = data;

  return keep_checksum_stats (info->status, info);
}

GList *
checkcopy_file_list_get_display_list (CheckcopyFileList * list)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (list);

  GList * file_list;

  file_list = g_hash_table_get_values (priv->files_hash);

  file_list = g_list_copy (file_list);

  if (priv->verify_only) {
    /* Filter out the checksums of the checksum files.
     * Usually you expect them not to have a checksum */
    file_list = ompa_list_filter (file_list, verify_list_filter);
  }

  return file_list;
}

CheckcopyFileListStats *
checkcopy_file_list_get_stats (CheckcopyFileList * list)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (list);

  CheckcopyFileListStats * stats;

  g_mutex_lock (priv->stats_mutex);
  stats = g_memdup (&(priv->stats), sizeof (CheckcopyFileListStats));
  g_mutex_unlock (priv->stats_mutex);

  return stats;
}

CheckcopyFileStatus
checkcopy_file_list_status_to_info (CheckcopyFileListCount i)
{
  switch (i) {
    case CHECKCOPY_FILE_LIST_COUNT_COPIED:
      return CHECKCOPY_STATUS_COPIED;
    case CHECKCOPY_FILE_LIST_COUNT_VERIFIED:
      return CHECKCOPY_STATUS_VERIFIED;
    case CHECKCOPY_FILE_LIST_COUNT_FAILED:
      return CHECKCOPY_STATUS_FAILED;
    case CHECKCOPY_FILE_LIST_COUNT_NOT_FOUND:
      return CHECKCOPY_STATUS_NOT_FOUND;
    case CHECKCOPY_FILE_LIST_COUNT_LAST:
      /* this case should never happen, fall back to the critical below */
      break;
  }
  g_critical ("Invalid file list status");
  return -1;
}

static void
mark_not_found (gpointer key, gpointer value, gpointer data)
{
  //gchar * relname = key;
  CheckcopyFileInfo * info = (CheckcopyFileInfo *) value;
  CheckcopyFileList * list = CHECKCOPY_FILE_LIST (data);

  g_assert (info != NULL);

  if (info->status == CHECKCOPY_STATUS_VERIFIABLE) {
    checkcopy_file_list_transition (list, info, CHECKCOPY_STATUS_NOT_FOUND);
  }
}

void
checkcopy_file_list_sweep (CheckcopyFileList * list)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (list);

  if (priv->verify_only) {
    DBG ("Marking files which were not found...");
    g_hash_table_foreach (priv->files_hash, mark_not_found, list);
  }
}

gboolean
checkcopy_file_list_transition (CheckcopyFileList * list,
                                CheckcopyFileInfo * info, CheckcopyFileStatus new_status)
{
  CheckcopyFileListPrivate *priv = GET_PRIVATE (list);

  gboolean r = FALSE;

  g_assert (list != NULL);
  g_assert (info != NULL);
  g_return_val_if_fail (new_status < CHECKCOPY_STATUS_LAST, FALSE);

  g_mutex_lock (priv->stats_mutex);

  switch (info->status) {
    case CHECKCOPY_STATUS_NONE:
      if (new_status == CHECKCOPY_STATUS_FOUND ||
          new_status == CHECKCOPY_STATUS_VERIFIABLE)
        r = TRUE;
      break;
    case CHECKCOPY_STATUS_VERIFIABLE:
      if (new_status == CHECKCOPY_STATUS_VERIFIED ||
          new_status == CHECKCOPY_STATUS_VERIFICATION_FAILED ||
          new_status == CHECKCOPY_STATUS_NOT_FOUND)
        r = TRUE;
      break;
    case CHECKCOPY_STATUS_FOUND:
      if (new_status == CHECKCOPY_STATUS_VERIFIED ||
          new_status == CHECKCOPY_STATUS_VERIFICATION_FAILED ||
          new_status ==  CHECKCOPY_STATUS_COPIED)
        r = TRUE;
      break;
    case CHECKCOPY_STATUS_NOT_FOUND:
      if (new_status == CHECKCOPY_STATUS_VERIFIED ||
          new_status == CHECKCOPY_STATUS_VERIFICATION_FAILED ||
          new_status == CHECKCOPY_STATUS_FAILED) {
        r = TRUE;
        priv->stats.count[CHECKCOPY_FILE_LIST_COUNT_NOT_FOUND]--;
      }
      break;
    case CHECKCOPY_STATUS_COPIED:
      if (new_status == CHECKCOPY_STATUS_VERIFIED ||
          new_status == CHECKCOPY_STATUS_VERIFICATION_FAILED ||
          new_status == CHECKCOPY_STATUS_FAILED) {
        r = TRUE;

        if (keep_checksum_stats (info->status, info)) {
          priv->stats.count[CHECKCOPY_FILE_LIST_COUNT_COPIED]--;
        }
      }
      break;
    case CHECKCOPY_STATUS_VERIFIED:
      if (new_status == CHECKCOPY_STATUS_FAILED) {
        r = TRUE;
        priv->stats.count[CHECKCOPY_FILE_LIST_COUNT_VERIFIED]--;
      }
      break;
    case CHECKCOPY_STATUS_VERIFICATION_FAILED:
      if (new_status == CHECKCOPY_STATUS_FAILED) {
        r = TRUE;
        priv->stats.count[CHECKCOPY_FILE_LIST_COUNT_FAILED]--;
      }
      break;
    case CHECKCOPY_STATUS_FAILED:
      if (new_status == CHECKCOPY_STATUS_FAILED) {
        r = TRUE;
        priv->stats.count[CHECKCOPY_FILE_LIST_COUNT_FAILED]--;
        g_warning ("Change from failed to failed is redundant");
      }
      break;
    case CHECKCOPY_STATUS_MARKER_PROCESSED:
    case CHECKCOPY_STATUS_LAST:
      /* these should never occur */
      g_critical ("Invalid current state %d", info->status);
      break;
  }

  switch (new_status) {
    case CHECKCOPY_STATUS_NONE:
    case CHECKCOPY_STATUS_VERIFIABLE:
    case CHECKCOPY_STATUS_FOUND:
      // do nothing
      break;
    case CHECKCOPY_STATUS_NOT_FOUND:
      priv->stats.count[CHECKCOPY_FILE_LIST_COUNT_NOT_FOUND]++;
      break;
    case CHECKCOPY_STATUS_COPIED:
      if (keep_checksum_stats (new_status, info)) {
        priv->stats.count[CHECKCOPY_FILE_LIST_COUNT_COPIED]++;
      }
      break;
    case CHECKCOPY_STATUS_VERIFIED:
      priv->stats.count[CHECKCOPY_FILE_LIST_COUNT_VERIFIED]++;
      break;
    case CHECKCOPY_STATUS_VERIFICATION_FAILED:
    case CHECKCOPY_STATUS_FAILED:
      priv->stats.count[CHECKCOPY_FILE_LIST_COUNT_FAILED]++;
      break;
    case CHECKCOPY_STATUS_MARKER_PROCESSED:
    case CHECKCOPY_STATUS_LAST:
      /* these should never occur */
      g_critical ("Invalid new state %d", new_status);
      break;
  }
  g_mutex_unlock (priv->stats_mutex);

  if (!r) {
    g_error ("Invalid state change: %d -> %d", info->status, new_status);
  } else {
    DBG ("Status change for %s from %d -> %d", info->relname, info->status, new_status);
    info->status = new_status;
  }

  return r;
}

CheckcopyFileList*
checkcopy_file_list_get_instance (void)
{
  return g_object_new (CHECKCOPY_TYPE_FILE_LIST, NULL);
}
