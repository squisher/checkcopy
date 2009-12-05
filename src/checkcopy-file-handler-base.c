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

#include <libxfce4util/libxfce4util.h>
#include "checkcopy-file-handler-base.h"

/*- private prototypes -*/

/*- globals -*/

enum {
  PROP_0,
  PROP_DESTINATION,
  PROP_PROGRESS_DIALOG,
  PROP_VERIFY_ONLY,
};


/*****************/
/*- class setup -*/
/*****************/

G_DEFINE_TYPE (CheckcopyFileHandlerBase, checkcopy_file_handler_base, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHECKCOPY_TYPE_FILE_HANDLER_BASE, CheckcopyFileHandlerBasePrivate))

typedef struct _CheckcopyFileHandlerBasePrivate CheckcopyFileHandlerBasePrivate;

struct _CheckcopyFileHandlerBasePrivate {
  int dummy;
};

static void
checkcopy_file_handler_base_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  CheckcopyFileHandlerBase * base = CHECKCOPY_FILE_HANDLER_BASE (object);
  //CheckcopyFileHandlerBasePrivate *priv = GET_PRIVATE (base);

  switch (property_id) {
    case PROP_DESTINATION:
      g_value_set_object (value, base->dest); 
      break;
    case PROP_PROGRESS_DIALOG:
      g_value_set_object (value, base->progress_dialog);
      break;
    case PROP_VERIFY_ONLY:
      g_value_set_boolean (value, base->verify_only);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
checkcopy_file_handler_base_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  CheckcopyFileHandlerBase * base = CHECKCOPY_FILE_HANDLER_BASE (object);
  //CheckcopyFileHandlerBasePrivate *priv = GET_PRIVATE (base);

  switch (property_id) {
    case PROP_DESTINATION:
      base->dest = g_value_dup_object (value); 
      break;
    case PROP_PROGRESS_DIALOG:
      base->progress_dialog = g_value_dup_object (value);
      break;
    case PROP_VERIFY_ONLY:
      base->verify_only = g_value_get_boolean (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
checkcopy_file_handler_base_finalize (GObject *object)
{
  CheckcopyFileHandlerBase *self = CHECKCOPY_FILE_HANDLER_BASE (object);
  //CheckcopyFileHandlerBasePrivate *priv = GET_PRIVATE (self);

  G_OBJECT_CLASS (checkcopy_file_handler_base_parent_class)->finalize (object);

  g_object_unref (self->dest);
  g_object_unref (self->progress_dialog);
  g_object_unref (self->list);
}

static void
checkcopy_file_handler_base_class_init (CheckcopyFileHandlerBaseClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CheckcopyFileHandlerBasePrivate));

  object_class->get_property = checkcopy_file_handler_base_get_property;
  object_class->set_property = checkcopy_file_handler_base_set_property;
  object_class->finalize = checkcopy_file_handler_base_finalize;

  g_object_class_install_property (object_class, PROP_DESTINATION,
           g_param_spec_object ("destination", _("Destination folder"), _("Destination folder"), G_TYPE_FILE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PROGRESS_DIALOG,
           g_param_spec_object ("progress-dialog", _("Progress dialog"), _("Progress dialog"), TYPE_PROGRESS_DIALOG, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_VERIFY_ONLY,
           g_param_spec_boolean ("verify-only", _("Only verify"), _("Only verify"), FALSE, G_PARAM_READWRITE));
}

static void
checkcopy_file_handler_base_init (CheckcopyFileHandlerBase *self)
{
  self->list = checkcopy_file_list_get_instance ();
}


/***************/
/*- internals -*/
/***************/

/*******************/
/*- public methods-*/
/*******************/

CheckcopyFileHandlerBase*
checkcopy_file_handler_base_new (void)
{
  return g_object_new (CHECKCOPY_TYPE_FILE_HANDLER_BASE, NULL);
}
