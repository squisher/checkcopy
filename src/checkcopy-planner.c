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

#include "checkcopy-planner.h"
#include "checkcopy-file-handler.h"


/*- private prototypes -*/

static void checkcopy_planner_file_handler_init (CheckcopyFileHandlerInterface *iface, gpointer data);
static void checkcopy_planner_finalize (GObject *obj);
static void process (CheckcopyFileHandler *fhandler, GFile *root, GFile *file, GFileInfo *info);
static const gchar * get_attribute_list (CheckcopyFileHandler  *fhandler);

/*- globals -*/

enum {
  PROP_0,
  PROP_TOTAL_SIZE,
  PROP_PROGRESS_DIALOG,
};


/*****************/
/*- class setup -*/
/*****************/

G_DEFINE_TYPE_EXTENDED (CheckcopyPlanner, checkcopy_planner, G_TYPE_OBJECT, 0, \
    G_IMPLEMENT_INTERFACE (CHECKCOPY_TYPE_FILE_HANDLER, checkcopy_planner_file_handler_init))

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHECKCOPY_TYPE_PLANNER, CheckcopyPlannerPrivate))

typedef struct _CheckcopyPlannerPrivate CheckcopyPlannerPrivate;

struct _CheckcopyPlannerPrivate {
  goffset size;
  ProgressDialog * progress_dialog;
};

static void
checkcopy_planner_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  CheckcopyPlannerPrivate *priv = GET_PRIVATE (CHECKCOPY_PLANNER (object));

  switch (property_id) {
    case PROP_TOTAL_SIZE:
      g_value_set_int64 (value, priv->size);
      break;
    case PROP_PROGRESS_DIALOG:
      g_value_set_object (value, priv->progress_dialog);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
checkcopy_planner_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  CheckcopyPlannerPrivate *priv = GET_PRIVATE (CHECKCOPY_PLANNER (object));

  switch (property_id) {
    case PROP_TOTAL_SIZE:
      priv->size = g_value_get_int64 (value);
      break;
    case PROP_PROGRESS_DIALOG:
      priv->progress_dialog = g_value_dup_object (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
checkcopy_planner_class_init (CheckcopyPlannerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CheckcopyPlannerPrivate));

  object_class->get_property = checkcopy_planner_get_property;
  object_class->set_property = checkcopy_planner_set_property;
  object_class->finalize = checkcopy_planner_finalize;

  g_object_class_install_property (object_class, PROP_TOTAL_SIZE,
           g_param_spec_int64 ("total-size", "Total size", "Total size", 0, G_MAXINT64, 0, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_PROGRESS_DIALOG,
           g_param_spec_object ("progress-dialog", "Progress dialog", "Progress dialog", TYPE_PROGRESS_DIALOG, G_PARAM_READWRITE));
}

static void
checkcopy_planner_file_handler_init (CheckcopyFileHandlerInterface *iface, gpointer data)
{
  iface->process = process;
  iface->get_attribute_list = get_attribute_list;
}

static void
checkcopy_planner_init (CheckcopyPlanner *self)
{
}

static void
checkcopy_planner_finalize (GObject *obj)
{
  CheckcopyPlanner * planner = CHECKCOPY_PLANNER (obj);
  CheckcopyPlannerPrivate *priv = GET_PRIVATE(planner);

  g_object_unref (priv->progress_dialog);
}


/***************/
/*- internals -*/
/***************/

static void
process (CheckcopyFileHandler *fhandler, GFile *root, GFile *file, GFileInfo *info)
{
  CheckcopyPlanner *planner = CHECKCOPY_PLANNER (fhandler);
  CheckcopyPlannerPrivate *priv = GET_PRIVATE(planner);

  priv->size += g_file_info_get_size (info);

  DBG ("After %s, total size is %llu", g_file_info_get_display_name (info), priv->size);

  g_object_set (priv->progress_dialog, "total-size", priv->size, NULL);
}

static const gchar *
get_attribute_list (CheckcopyFileHandler *fhandler)
{
  return G_FILE_ATTRIBUTE_STANDARD_SIZE;
}

/*******************/
/*- public methods-*/
/*******************/

CheckcopyPlanner*
checkcopy_planner_new (ProgressDialog * progress_dialog)
{
  return g_object_new (CHECKCOPY_TYPE_PLANNER, "progress-dialog", progress_dialog, NULL);
}
