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

#include <zlib.h>

#include <glib.h>

#include "checkcopy-input-stream.h"

typedef struct {
  gboolean in_use;
  uLong raw;
  char * string;
} Crc32Data;

typedef union {
  GChecksum *glib;
  Crc32Data zlib;
} ChecksumData;

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

static ChecksumData checksum_new (CheckcopyChecksumType type);
static void checksum_update (CheckcopyChecksumType type, ChecksumData *data,
                             void *buffer, gssize nread);
static const char * checksum_get_string (CheckcopyChecksumType type, ChecksumData data);

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
  ChecksumData checksum[CHECKCOPY_ALL_CHECKSUMS];
};

static void
checkcopy_input_stream_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  //CheckcopyInputStreamPrivate *priv = GET_PRIVATE (CHECKCOPY_INPUT_STREAM (object));

  switch (property_id) {
#if 0
    case PROP_CHECKSUM_TYPE:
      g_value_set_int (value, priv->type);
      break;
#endif
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
    {
      gint type;

      type = g_value_get_int (value);

      if (type != CHECKCOPY_ALL_CHECKSUMS) {
        priv->checksum[type] = checksum_new (type);
      } else {
        int i;

        for (i=CHECKCOPY_NO_CHECKSUM+1; i<CHECKCOPY_ALL_CHECKSUMS; i++) {
          priv->checksum[i] = checksum_new (i);
        }
      }
    } break;
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
                             0, G_MAXINT, G_CHECKSUM_SHA1,
                             G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

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

  /* Note that priv->checksum is initialized in the setter */

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

  int i;

  for (i=CHECKCOPY_NO_CHECKSUM+1; i<CHECKCOPY_ALL_CHECKSUMS; i++) {
    if (i!=CHECKCOPY_CRC32) {
      g_checksum_free (priv->checksum[i].glib);
    } else {
      g_free (priv->checksum[i].zlib.string);
    }
  }
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
  int i;

  nread = g_input_stream_read (base_stream, buffer, count, cancellable, error);

  for (i=CHECKCOPY_NO_CHECKSUM+1; i<CHECKCOPY_ALL_CHECKSUMS; i++) {
    checksum_update (i, &(priv->checksum[i]), buffer, nread);
  }

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

static ChecksumData
checksum_new (CheckcopyChecksumType type)
{
  ChecksumData data;

  switch (type) {
    case CHECKCOPY_MD5:
      data.glib = g_checksum_new (G_CHECKSUM_MD5);
      break;
    case CHECKCOPY_SHA1:
      data.glib = g_checksum_new (G_CHECKSUM_SHA1);
      break;
    case CHECKCOPY_SHA256:
      data.glib = g_checksum_new (G_CHECKSUM_SHA256);
      break;
    case CHECKCOPY_CRC32:
      data.zlib.in_use = TRUE;
      data.zlib.raw = crc32 (0L, NULL, 0);
      data.zlib.string = NULL;
      break;
    default:
      g_critical ("Invalid checksum type");
  }

  return data;
}

static void
checksum_update (CheckcopyChecksumType type, ChecksumData *data,
                 void *buffer, gssize nread)
{
  switch (type) {
    case CHECKCOPY_MD5:
    case CHECKCOPY_SHA1:
    case CHECKCOPY_SHA256:
      if (data->glib) {
        g_checksum_update (data->glib, buffer, nread);
      }
      break;
    case CHECKCOPY_CRC32:
      if (data->zlib.in_use) {
        data->zlib.raw = crc32 (data->zlib.raw, buffer, nread);
      }
      break;
    default:
      g_critical ("Invalid checksum type");
  }
}

static const char *
checksum_get_string (CheckcopyChecksumType type, ChecksumData data)
{
  switch (type) {
    case CHECKCOPY_MD5:
    case CHECKCOPY_SHA1:
    case CHECKCOPY_SHA256:
      if (data.glib) {
        return g_checksum_get_string (data.glib);
      } else {
        g_critical ("Requested checksum which was not computed");

        return NULL;
      }
    case CHECKCOPY_CRC32:
      if (data.zlib.string) {
        return data.zlib.string;
      } else if (data.zlib.raw != 0L) {
        data.zlib.string = g_strdup_printf ("%08x", (guint) data.zlib.raw);

        return data.zlib.string;
      } else {
        return NULL;
      }
    default:
      g_critical ("Invalid checksum type");
      return NULL;
  }
}


/*******************/
/*- public methods-*/
/*******************/

const gchar *
checkcopy_input_stream_get_checksum (CheckcopyInputStream * stream, CheckcopyChecksumType type)
{
  CheckcopyInputStreamPrivate *priv = GET_PRIVATE (stream);
  GFilterInputStream *filter = G_FILTER_INPUT_STREAM (stream);
  GInputStream *base = filter->base_stream;

  const gchar *checksum;

  if (g_input_stream_is_closed (base)) {
    checksum = checksum_get_string (type, priv->checksum[type]);
  } else {
    checksum = NULL;
  }

  return checksum;
}

CheckcopyInputStream*
checkcopy_input_stream_new (GInputStream *in, CheckcopyChecksumType type)
{
  return g_object_new (CHECKCOPY_TYPE_INPUT_STREAM,
                       "base-stream", in,
                       "checksum-type", type,
                       NULL);
}
