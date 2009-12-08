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

#include <libxfce4util/libxfce4util.h>

#include <gio/gio.h>

#include "error.h"
#include "checkcopy-processor.h"
#include "checkcopy-file-handler.h"
#include "checkcopy-file-handler-base.h"
#include "checkcopy-file-info.h"
#include "checkcopy-file-list.h"
#include "checkcopy-cancel.h"
#include "checkcopy-input-stream.h"

/*- private prototypes -*/

static void checkcopy_processor_file_handler_init (CheckcopyFileHandlerInterface *iface, gpointer data);
static void checkcopy_processor_finalize (GObject *obj);
static void process (CheckcopyFileHandler *fhandler, GFile *root, GFile *file, GFileInfo *info);
static const gchar * get_attribute_list (CheckcopyFileHandler  *fhandler);

static void process_directory (CheckcopyProcessor *proc, GFileInfo * info, GFile * dst, gchar * relname, gboolean verify_only);
static void process_file (CheckcopyProcessor *proc, GFile *file, GFileInfo *info, GFile * dst, gchar * relname, gboolean verify_only);
static gboolean verify_file (CheckcopyProcessor *proc, GFile * file, CheckcopyInputStream *cin);
static gboolean copy_file (CheckcopyProcessor *proc, CheckcopyInputStream *cin, GFile * dst);

/*- globals -*/

enum {
  PROP_0,
};


/*****************/
/*- class setup -*/
/*****************/

G_DEFINE_TYPE_EXTENDED (CheckcopyProcessor, checkcopy_processor, CHECKCOPY_TYPE_FILE_HANDLER_BASE, 0, \
                        G_IMPLEMENT_INTERFACE (CHECKCOPY_TYPE_FILE_HANDLER, checkcopy_processor_file_handler_init))

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHECKCOPY_TYPE_PROCESSOR, CheckcopyProcessorPrivate))

typedef struct _CheckcopyProcessorPrivate CheckcopyProcessorPrivate;

struct _CheckcopyProcessorPrivate {
  int dummy;
};

static void
checkcopy_processor_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  //CheckcopyProcessorPrivate *priv = GET_PRIVATE (CHECKCOPY_PROCESSOR (object));

  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
checkcopy_processor_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  //CheckcopyProcessorPrivate *priv = GET_PRIVATE (CHECKCOPY_PROCESSOR (object));

  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
checkcopy_processor_class_init (CheckcopyProcessorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CheckcopyProcessorPrivate));

  object_class->get_property = checkcopy_processor_get_property;
  object_class->set_property = checkcopy_processor_set_property;
  object_class->finalize = checkcopy_processor_finalize;
}

static void
checkcopy_processor_file_handler_init (CheckcopyFileHandlerInterface *iface, gpointer data)
{
  iface->process = process;
  iface->get_attribute_list = get_attribute_list;
}

static void
checkcopy_processor_init (CheckcopyProcessor *self)
{
  //CheckcopyProcessorPrivate * priv = GET_PRIVATE (self);
}

static void
checkcopy_processor_finalize (GObject *obj)
{
  G_OBJECT_CLASS (checkcopy_processor_parent_class)->finalize (obj);
}


/***************/
/*- internals -*/
/***************/

/* this function is based on g_output_stream_splice */
static gssize  
splice (CheckcopyProcessor *proc, GOutputStream *stream, CheckcopyInputStream *in, GCancellable *cancellable, GError **error)
{
  //CheckcopyProcessorPrivate *priv = GET_PRIVATE (CHECKCOPY_PROCESSOR (proc));
  CheckcopyFileHandlerBase *base = CHECKCOPY_FILE_HANDLER_BASE (proc);

  GInputStream *source;
  gssize n_read, n_written;
  gssize bytes_copied;
  char buffer[8192], *p;
  gboolean res;

  g_assert (stream != NULL);
  g_assert (in != NULL);
  g_assert (error == NULL || *error == NULL);

  source = G_INPUT_STREAM (in);

  bytes_copied = 0;
  
  res = TRUE;
  do 
    {
      n_read = g_input_stream_read (source, buffer, sizeof (buffer), cancellable, error);
      if (n_read == -1)
        {
          res = FALSE;
          break;
        }
  
      if (n_read == 0)
        break;

      p = buffer;
      while (n_read > 0)
        {
          n_written = g_output_stream_write (stream, p, n_read, cancellable, error);

          progress_dialog_thread_add_size (base->progress_dialog, n_written);

          if (n_written == -1)
            {
              res = FALSE;
              break;
            }

          p += n_written;
          n_read -= n_written;
          bytes_copied += n_written;
        }
    }
  while (res);

  g_input_stream_close (source, cancellable, NULL);
  g_output_stream_close (stream, cancellable, NULL);

  if (res)
    return bytes_copied;

  return -1;
}


static void
process_directory (CheckcopyProcessor *proc, GFileInfo * info, GFile * dst, gchar * relname, gboolean verify_only)
{
  //CheckcopyProcessorPrivate *priv = GET_PRIVATE (CHECKCOPY_PROCESSOR (proc));
  CheckcopyFileHandlerBase *base = CHECKCOPY_FILE_HANDLER_BASE (proc);

  GCancellable * cancel = checkcopy_get_cancellable ();
  GError *error = NULL;
  gboolean had_error = FALSE;

  if (verify_only) {
    /* do nothing */
    return;
  }

  DBG ("Mkdir %s", relname);

  if (!g_cancellable_set_error_if_cancelled (cancel, &error))
    g_file_make_directory (dst, cancel, &error);

  if (error) {
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
      /* not an error */
    } else {
      thread_show_gerror (dst, error);
      g_error_free (error);
      had_error = TRUE;
    }
  }

  if (!had_error) {
    progress_dialog_thread_add_size (base->progress_dialog, g_file_info_get_size (info));
  }
}


static gboolean
copy_file (CheckcopyProcessor *proc, CheckcopyInputStream *cin, GFile * dst)
{
  //CheckcopyProcessorPrivate *priv = GET_PRIVATE (CHECKCOPY_PROCESSOR (proc));

  GCancellable * cancel = checkcopy_get_cancellable ();
  GError *error = NULL;
  gboolean r;

  GOutputStream *out = NULL;

  if (!g_cancellable_set_error_if_cancelled (cancel, &error))
    out = G_OUTPUT_STREAM (g_file_replace (dst, NULL, FALSE, 0, cancel, &error));

  if (out == NULL || error) {
    thread_show_gerror (dst, error);
    g_error_free (error);
    error = NULL;

    r = FALSE;

  } else {
    /* created output stream */
    if (!g_cancellable_set_error_if_cancelled (cancel, &error))
      splice (proc, out, cin, cancel, &error);

    if (error) {
      thread_show_gerror (dst, error);
      g_error_free (error);
      error = NULL;

      r = FALSE;
    } else {
      r = TRUE;
    }

    g_object_unref (out);
  }

  return r;
}


static gboolean
verify_file (CheckcopyProcessor *proc, GFile * file, CheckcopyInputStream *cin)
{
  //CheckcopyProcessorPrivate *priv = GET_PRIVATE (CHECKCOPY_PROCESSOR (proc));
  CheckcopyFileHandlerBase *base = CHECKCOPY_FILE_HANDLER_BASE (proc);

  gboolean r = TRUE;
  GInputStream *source;
  gssize n_read;
  char buffer[8192];
  GError * error = NULL;
  GCancellable * cancel = checkcopy_get_cancellable ();

  g_assert (cin != NULL);

  source = G_INPUT_STREAM (cin);

  do {
    n_read = g_input_stream_read (source, buffer, sizeof (buffer), cancel, &error);

    progress_dialog_thread_add_size (base->progress_dialog, n_read);

    if (n_read == -1) {
      thread_show_gerror (file, error);
      g_error_free (error);
      error = NULL;

      r = FALSE;
    }
    
    if (n_read == 0) {
      break;
    }
  } while (r);

  g_input_stream_close (source, cancel, NULL);

  return r;
}


static void 
process_file (CheckcopyProcessor *proc, GFile *file, GFileInfo *info, GFile * dst, gchar * relname, gboolean verify_only)
{
  //CheckcopyProcessorPrivate *priv = GET_PRIVATE (CHECKCOPY_PROCESSOR (proc));
  CheckcopyFileHandlerBase *base = CHECKCOPY_FILE_HANDLER_BASE (proc);

  GCancellable * cancel = checkcopy_get_cancellable ();
  GError *error = NULL;

  CheckcopyInputStream *cin;
  GInputStream *in = NULL;
  const gchar *checksum;
  CheckcopyChecksumType checksum_type;

  /* we assume it is a file */

  DBG ("Copy  %s", relname);

#if 0
#ifdef DEBUG
  {
    gchar *uri = g_file_get_uri (file);
    DBG ("Src = %s", uri);
    g_free (uri);
  }
#endif
#endif

  if (!g_cancellable_set_error_if_cancelled (cancel, &error))
    in = G_INPUT_STREAM (g_file_read (file, cancel, &error));

  if (in == NULL || error) {
    thread_show_gerror (file, error);
    g_error_free (error);
    error = NULL;

  } else {
    /* created input stream successfully */

    gboolean r;

    checksum_type = checkcopy_file_list_get_file_type (base->list, relname);
    if (checksum_type == CHECKCOPY_NO_CHECKSUM)
      checksum_type = CHECKCOPY_SHA1;

    cin = checkcopy_input_stream_new (in, checkcopy_checksum_type_to_gio (checksum_type));

    if (verify_only) {
      r = verify_file (proc, file, cin);
    } else {
      r = copy_file (proc, cin, dst);
    }

    if (r) {
      checksum = checkcopy_input_stream_get_checksum (cin);

      checkcopy_file_list_check_file (base->list, relname, checksum, checksum_type);
    }

    g_object_unref (in);
    g_object_unref (cin);
  }
}


static void
process (CheckcopyFileHandler *fhandler, GFile *root, GFile *file, GFileInfo *info)
{
  CheckcopyProcessor *proc = CHECKCOPY_PROCESSOR (fhandler);
  //CheckcopyProcessorPrivate *priv = GET_PRIVATE (CHECKCOPY_PROCESSOR (proc));
  CheckcopyFileHandlerBase *base = CHECKCOPY_FILE_HANDLER_BASE (proc);

  gchar *relname;
  GFile *dst = NULL;

  relname = g_file_get_relative_path (root, file);

  if (checkcopy_file_list_get_status (base->list, relname) > CHECKCOPY_STATUS_MARKER_PROCESSED) {
    DBG ("%s is already processed.", relname);
    g_free (relname);
    return;
  }


  progress_dialog_thread_set_filename (base->progress_dialog, relname);

  if (!base->verify_only) {
    dst = g_file_resolve_relative_path (base->dest, relname);
  } else {
    dst = g_object_ref (G_OBJECT (base->dest));
  }

#ifdef DEBUG
  {
    gchar *root_uri = g_file_get_uri (root);
    gchar *file_uri = g_file_get_uri (file);
    gchar *dest_uri = g_file_get_uri (dst);

    //DBG ("root = %s, file = %s, dest = %s", root_uri, file_uri, dest_uri);

    g_free (root_uri);
    g_free (file_uri);
    g_free (dest_uri);
  }
#endif

  if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY) {
    process_directory (proc, info, dst, relname, base->verify_only);

  } else {
    process_file (proc, file, info, dst, relname, base->verify_only);
  }

  g_object_unref (dst);
  g_free (relname);
}

static const gchar *
get_attribute_list (CheckcopyFileHandler  *fhandler)
{
  return G_FILE_ATTRIBUTE_STANDARD_SIZE;
}

/*******************/
/*- public methods-*/
/*******************/

CheckcopyProcessor*
checkcopy_processor_new (ProgressDialog * progress_dialog, GFile *dest, gboolean verify_only)
{
  return g_object_new (CHECKCOPY_TYPE_PROCESSOR, "progress-dialog", progress_dialog, "destination", dest, "verify-only", verify_only, NULL);
}
