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

#include "checkcopy-input-stream.h"

/*- private prototypes -*/

static GObject * checkcopy_input_stream_construct (GType type,
                                                   guint n_construct_properties,
                                                   GObjectConstructParam * construct_properties);
static void checkcopy_input_stream_finalize (GObject *obj);
static gssize   read_fn      (GInputStream        *stream,
                              void                *buffer,
                              gsize                count,
                              GCancellable        *cancellable,
                              GError             **error);
#if 0
static gssize   skip         (GInputStream        *stream,
                              gsize                count,
                              GCancellable        *cancellable,
                              GError             **error);
static gboolean close_fn     (GInputStream        *stream,
                              GCancellable        *cancellable,
                              GError             **error);
#endif


/*- globals -*/

static GFilterInputStreamClass * parent_class = NULL;

enum {
  PROP_0,
  PROP_CHECKSUM_TYPE,
};


/*****************/
/*- class setup -*/
/*****************/

G_DEFINE_TYPE (CheckcopyInputStream, checkcopy_input_stream, G_TYPE_FILTER_INPUT_STREAM)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHECKCOPY_TYPE_INPUT_STREAM, CheckcopyInputStreamPrivate))

typedef struct _CheckcopyInputStreamPrivate CheckcopyInputStreamPrivate;

struct _CheckcopyInputStreamPrivate {
  GChecksum *checksum;
  GChecksumType type;
};

static void
checkcopy_input_stream_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  CheckcopyInputStreamPrivate *priv = GET_PRIVATE (CHECKCOPY_INPUT_STREAM (object));

  switch (property_id) {
    case PROP_CHECKSUM_TYPE:
      g_value_set_int (value, priv->type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
checkcopy_input_stream_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  CheckcopyInputStreamPrivate *priv = GET_PRIVATE (CHECKCOPY_INPUT_STREAM (object));

  switch (property_id) {
    case PROP_CHECKSUM_TYPE:
      priv->type = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
checkcopy_input_stream_class_init (CheckcopyInputStreamClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GInputStreamClass *in_class = G_INPUT_STREAM_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CheckcopyInputStreamPrivate));
  parent_class = g_type_class_peek_parent (klass);

  object_class->get_property = checkcopy_input_stream_get_property;
  object_class->set_property = checkcopy_input_stream_set_property;

  object_class->finalize = checkcopy_input_stream_finalize;
  object_class->constructor = checkcopy_input_stream_construct;

  g_object_class_install_property (object_class, PROP_CHECKSUM_TYPE,
           g_param_spec_int ("checksum-type", _("Checksum type"), _("Checksum type"), 
                             0, G_MAXINT, G_CHECKSUM_SHA1, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  in_class->read_fn = read_fn;
  //in_class->skip = skip;
  //in_class->close_fn = close_fn;
}

static GObject *
checkcopy_input_stream_construct (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties)
{
  GObject *obj;
  CheckcopyInputStreamPrivate *priv;

  obj = G_OBJECT_CLASS (parent_class)->constructor (type, n_construct_properties, construct_properties);
  priv = GET_PRIVATE (CHECKCOPY_INPUT_STREAM (obj));

  //DBG ("Using checksum type %d, sha1 = %d", priv->type, G_CHECKSUM_SHA1);

  priv->checksum = g_checksum_new (priv->type);

  return obj;
}

static void
checkcopy_input_stream_init (CheckcopyInputStream * self)
{
}

static void
checkcopy_input_stream_finalize (GObject *obj)
{
  CheckcopyInputStream * stream = CHECKCOPY_INPUT_STREAM (obj);
  CheckcopyInputStreamPrivate * priv = GET_PRIVATE (CHECKCOPY_INPUT_STREAM (stream));

  g_checksum_free (priv->checksum);
}

/***************/
/*- internals -*/
/***************/

gssize   read_fn      (GInputStream        *stream,
                       void                *buffer,
                       gsize                count,
                       GCancellable        *cancellable,
                       GError             **error)
{
  CheckcopyInputStreamPrivate *priv = GET_PRIVATE (CHECKCOPY_INPUT_STREAM (stream));
  GFilterInputStream *filter_stream = G_FILTER_INPUT_STREAM (stream);
  GInputStream *base_stream = filter_stream->base_stream;
  gssize nread;

  nread = g_input_stream_read (base_stream, buffer, count, cancellable, error);

  g_checksum_update (priv->checksum, buffer, nread);

  return nread;
}

#if 0
gssize   skip         (GInputStream        *stream,
                       gsize                count,
                       GCancellable        *cancellable,
                       GError             **error)
{
  GFilterInputStream *filter_stream = G_FILTER_INPUT_STREAM (stream);
  GInputStream *base_stream = filter_stream->base_stream;

  return g_input_stream_skip (base_stream, count, cancellable, error);
}

gboolean close_fn     (GInputStream        *stream,
                       GCancellable        *cancellable,
                       GError             **error)
{
  return TRUE;
}
#endif


/*******************/
/*- public methods-*/
/*******************/

const gchar *
checkcopy_input_stream_get_checksum (CheckcopyInputStream * stream)
{
  CheckcopyInputStreamPrivate *priv = GET_PRIVATE (stream);
  GFilterInputStream *filter = G_FILTER_INPUT_STREAM (stream);
  GInputStream *base = filter->base_stream;

  const gchar *checksum;

  if (g_input_stream_is_closed (base)) {
    checksum = g_checksum_get_string (priv->checksum);
  } else {
    checksum = NULL;
  }

  return checksum;
}

CheckcopyInputStream*
checkcopy_input_stream_new (GInputStream *in, GChecksumType type)
{
  return g_object_new (CHECKCOPY_TYPE_INPUT_STREAM,
                       "base-stream", in,
                       "checksum-type", type,
                       NULL);
}
