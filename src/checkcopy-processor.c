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


#include "checkcopy-processor.h"
#include "checkcopy-file-handler.h"

/*- private prototypes -*/

static void checkcopy_processor_file_handler_init (CheckcopyFileHandlerInterface *iface, gpointer data);
static void process (CheckcopyFileHandler *fhandler, GFile *root, GFile *file, GFileInfo *info);
static const gchar * get_attribute_list (CheckcopyFileHandler  *fhandler);

/*- globals -*/

enum {
  PROP_0,
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
checkcopy_processor_new (void)
{
  return g_object_new (CHECKCOPY_TYPE_PROCESSOR, NULL);
}
