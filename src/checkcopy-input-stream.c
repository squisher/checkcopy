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

#include "checkcopy-input-stream.h"

/*- private prototypes -*/

const gchar * get_checkcopy_string (CheckcopyInputStream *stream);

/*- globals -*/

enum {
  PROP_0,
  PROP_CHECKSUM,
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
  int dummy;
};

static void
checkcopy_input_stream_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  CheckcopyInputStreamPrivate *priv = GET_PRIVATE (CHECKCOPY_INPUT_STREAM (object));

  switch (property_id) {
    case PROP_CHECKSUM:
      g_value_set_static_string (value, get_checkcopy_string(CHECKCOPY_INPUT_STREAM (object)));
      break;
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

  g_type_class_add_private (klass, sizeof (CheckcopyInputStreamPrivate));

  object_class->get_property = checkcopy_input_stream_get_property;
  object_class->set_property = checkcopy_input_stream_set_property;

  g_object_class_install_property (object_class, PROP_CHECKSUM,
           g_param_spec_string ("checksum", "Checksum", "Checksum", NULL, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_CHECKSUM_TYPE,
           g_param_spec_int ("checksum-type", "Checksum type", "Checksum type", 0, G_MAXINT, G_CHECKSUM_SHA1, G_PARAM_READWRITE));
}

static void
checkcopy_input_stream_init (CheckcopyInputStream *self)
{
}


/***************/
/*- internals -*/
/***************/

const gchar *
get_checkcopy_string (CheckcopyInputStream *stream)
{
  CheckcopyInputStreamPrivate *priv = GET_PRIVATE (stream);

  GError *error = NULL;
  const gchar *str;

  str = g_checksum_get_string (priv->checksum);

  g_input_stream_close (G_FILTER_INPUT_STREAM(stream)->base_stream, NULL, &error);
  g_input_stream_close (G_INPUT_STREAM (stream), NULL, &error);

  return str;
}

/*******************/
/*- public methods-*/
/*******************/

CheckcopyInputStream*
checkcopy_input_stream_new (void)
{
  return g_object_new (CHECKCOPY_TYPE_INPUT_STREAM, NULL);
}
