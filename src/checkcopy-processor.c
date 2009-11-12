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

#include "checkcopy-processor.h"
#include "checkcopy-file-handler.h"
#include "checkcopy-cancel.h"
#include "checkcopy-input-stream.h"

/*- private prototypes -*/

static void checkcopy_processor_file_handler_init (CheckcopyFileHandlerInterface *iface, gpointer data);
static void checkcopy_processor_finalize (GObject *obj);
static void process (CheckcopyFileHandler *fhandler, GFile *root, GFile *file, GFileInfo *info);
static const gchar * get_attribute_list (CheckcopyFileHandler  *fhandler);

/*- globals -*/

enum {
  PROP_0,
  PROP_DESTINATION,
  PROP_PROGRESS_DIALOG,
};


/*****************/
/*- class setup -*/
/*****************/

G_DEFINE_TYPE_EXTENDED (CheckcopyProcessor, checkcopy_processor, G_TYPE_OBJECT, 0, \
                        G_IMPLEMENT_INTERFACE (CHECKCOPY_TYPE_FILE_HANDLER, checkcopy_processor_file_handler_init))

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHECKCOPY_TYPE_PROCESSOR, CheckcopyProcessorPrivate))

typedef struct _CheckcopyProcessorPrivate CheckcopyProcessorPrivate;

struct _CheckcopyProcessorPrivate {
  GFile *dest;
  ProgressDialog * progress_dialog;
};

static void
checkcopy_processor_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  CheckcopyProcessorPrivate *priv = GET_PRIVATE (CHECKCOPY_PROCESSOR (object));

  switch (property_id) {
    case PROP_DESTINATION:
      g_value_set_object (value, priv->dest); 
      break;
    case PROP_PROGRESS_DIALOG:
      g_value_set_object (value, priv->progress_dialog);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
checkcopy_processor_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  CheckcopyProcessorPrivate *priv = GET_PRIVATE (CHECKCOPY_PROCESSOR (object));

  switch (property_id) {
    case PROP_DESTINATION:
      priv->dest = g_value_dup_object (value); 
      break;
    case PROP_PROGRESS_DIALOG:
      priv->progress_dialog = g_value_dup_object (value);
      break;
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

  g_object_class_install_property (object_class, PROP_DESTINATION,
           g_param_spec_object ("destination", "Destination folder", "Destination folder", G_TYPE_FILE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PROGRESS_DIALOG,
           g_param_spec_object ("progress-dialog", "Progress dialog", "Progress dialog", TYPE_PROGRESS_DIALOG, G_PARAM_READWRITE));
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
}

static void
checkcopy_processor_finalize (GObject *obj)
{
  CheckcopyProcessor *self = CHECKCOPY_PROCESSOR (obj);
  CheckcopyProcessorPrivate *priv = GET_PRIVATE (CHECKCOPY_PROCESSOR (self));

  g_object_unref (priv->dest);
  g_object_unref (priv->progress_dialog);
}


/***************/
/*- internals -*/
/***************/

/* this function is based on g_output_stream_splice */
static gssize  
splice (CheckcopyProcessor *proc, GOutputStream *stream, CheckcopyInputStream *in, GCancellable *cancellable, GError **error)
{
  CheckcopyProcessorPrivate *priv = GET_PRIVATE (CHECKCOPY_PROCESSOR (proc));

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

          progress_dialog_thread_add_size (priv->progress_dialog, n_written);

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

  // FIXME: handle errors
  g_input_stream_close (source, cancellable, NULL);
  g_output_stream_close (stream, cancellable, NULL);

  if (res)
    return bytes_copied;

  return -1;
}

static void
process (CheckcopyFileHandler *fhandler, GFile *root, GFile *file, GFileInfo *info)
{
  CheckcopyProcessor *proc = CHECKCOPY_PROCESSOR (fhandler);
  CheckcopyProcessorPrivate *priv = GET_PRIVATE (CHECKCOPY_PROCESSOR (proc));

  GCancellable * cancel = checkcopy_get_cancellable ();
  gchar *relname;
  GError *error = NULL;
  GFile *dst;

  relname = g_file_get_relative_path (root, file);
  /* 
  if (relname == NULL)
    relname = g_file_get_basename (file);
  */

  progress_dialog_thread_set_filename (priv->progress_dialog, relname);

  dst = g_file_resolve_relative_path (priv->dest, relname);

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

    DBG ("Mkdir %s", relname);
    /* TODO: handle errors */
    g_file_make_directory (dst, cancel, &error);


  } else {
    CheckcopyInputStream *cin;
    GInputStream *in;
    GOutputStream *out;
    const gchar *checksum;

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

    /* TODO: handle errors */
    /*
    g_file_copy (file, dst, G_FILE_COPY_NONE, cancel, 
                 NULL, NULL,
                 &error);
                 */
    in = G_INPUT_STREAM (g_file_read (file, cancel, &error));

    if (in == NULL) {
      DBG ("Could not open input file: %s", error->message);
      /* TODO: add to error list */
      return;
    }

    cin = checkcopy_input_stream_new (in, G_CHECKSUM_SHA1);

    out = G_OUTPUT_STREAM (g_file_replace (dst, NULL, FALSE, 0, cancel, &error));

    if (out == NULL) {
      DBG ("Could not create destination file: %s", error->message);
      /* TODO: add to error list */
      return;
    }

    /*
    g_output_stream_splice (out, G_INPUT_STREAM (cin), 
                            G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                            cancel, &error);
                            */
    splice (proc, out, cin, cancel, &error);

    checksum = checkcopy_input_stream_get_checksum (cin);
    DBG ("Checksum: %s", checksum);

    g_object_unref (out);
    g_object_unref (in);
    g_object_unref (cin);
  }

  g_free (relname);
}

static const gchar *
get_attribute_list (CheckcopyFileHandler  *fhandler)
{
  return "";
}

/*******************/
/*- public methods-*/
/*******************/

CheckcopyProcessor*
checkcopy_processor_new (ProgressDialog * progress_dialog, GFile *dest)
{
  return g_object_new (CHECKCOPY_TYPE_PROCESSOR, "progress-dialog", progress_dialog, "destination", dest, NULL);
}
