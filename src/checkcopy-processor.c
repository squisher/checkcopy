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
static void process (CheckcopyFileHandler *fhandler, GFile *root, GFile *file, GFileInfo *info);
static const gchar * get_attribute_list (CheckcopyFileHandler  *fhandler);

/*- globals -*/

enum {
  PROP_0,
  PROP_DESTINATION,
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
      priv->dest = g_value_get_object (value); 
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

  g_object_class_install_property (object_class, PROP_DESTINATION,
           g_param_spec_object ("destination", "Destination folder", "Destination folder", G_TYPE_FILE, G_PARAM_READWRITE));
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


/***************/
/*- internals -*/
/***************/

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

    g_output_stream_splice (out, G_INPUT_STREAM (cin), 
                            G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                            cancel, &error);

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
checkcopy_processor_new (GFile *dest)
{
  return g_object_new (CHECKCOPY_TYPE_PROCESSOR, "destination", dest, NULL);
}
