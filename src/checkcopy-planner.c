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
#include "checkcopy-file-info.h"
#include "checkcopy-file-list.h"
#include "checkcopy-file-handler-base.h"


/*- private prototypes -*/

static void checkcopy_planner_file_handler_init (CheckcopyFileHandlerInterface *iface, gpointer data);
static void checkcopy_planner_finalize (GObject *obj);
static void process (CheckcopyFileHandler *fhandler, GFile *root, GFile *file, GFileInfo *info);
static const gchar * get_attribute_list (CheckcopyFileHandler  *fhandler);

/*- globals -*/

enum {
  PROP_0,
  PROP_TOTAL_SIZE,
  PROP_NUM_FILES,
};


/*****************/
/*- class setup -*/
/*****************/

G_DEFINE_TYPE_EXTENDED (CheckcopyPlanner, checkcopy_planner, CHECKCOPY_TYPE_FILE_HANDLER_BASE, 0, \
    G_IMPLEMENT_INTERFACE (CHECKCOPY_TYPE_FILE_HANDLER, checkcopy_planner_file_handler_init))

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHECKCOPY_TYPE_PLANNER, CheckcopyPlannerPrivate))

typedef struct _CheckcopyPlannerPrivate CheckcopyPlannerPrivate;

struct _CheckcopyPlannerPrivate {
  goffset size;
  guint num_files;
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
    case PROP_NUM_FILES:
      g_value_set_uint (value, priv->num_files);
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
           g_param_spec_int64 ("total-size", _("Total size"), _("Total size"), 0, G_MAXINT64, 0, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_NUM_FILES,
           g_param_spec_uint ("num-files", _("Number of files"), _("Number of files"), 0, G_MAXUINT, 0, G_PARAM_READABLE));
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
  //CheckcopyPlannerPrivate *priv = GET_PRIVATE(self);
}

static void
checkcopy_planner_finalize (GObject *obj)
{
  //CheckcopyPlanner * planner = CHECKCOPY_PLANNER (obj);
  //CheckcopyPlannerPrivate *priv = GET_PRIVATE(planner);

  G_OBJECT_CLASS (checkcopy_planner_parent_class)->finalize (obj);
}


/***************/
/*- internals -*/
/***************/

static void
process (CheckcopyFileHandler *fhandler, GFile *root, GFile *file, GFileInfo *info)
{
  CheckcopyPlanner *planner = CHECKCOPY_PLANNER (fhandler);
  CheckcopyFileHandlerBase *base = CHECKCOPY_FILE_HANDLER_BASE (fhandler);
  CheckcopyPlannerPrivate *priv = GET_PRIVATE(planner);

  gchar * relname;
  CheckcopyFileInfo * our_info;

  relname = g_file_get_relative_path (root, file);
  our_info = checkcopy_file_list_grab_info (base->list, relname);

  if (!our_info->seen) {

    our_info->seen = TRUE;

    if (our_info->status == CHECKCOPY_STATUS_NONE)
      checkcopy_file_list_transition (base->list, our_info, CHECKCOPY_STATUS_FOUND);

    if (g_file_info_get_file_type (info) != G_FILE_TYPE_DIRECTORY) {
      priv->num_files++;
    }

    priv->size += g_file_info_get_size (info);

    DBG ("After %s, total size is %llu", g_file_info_get_display_name (info), priv->size);

    g_object_set (base->progress_dialog, "total-size", priv->size, NULL);


    if (checkcopy_file_info_is_checksum_file (our_info, file)) {
      checksum_file_list_parse_checksum_file (base->list, root, file);
    }
  }

  g_free (relname);
}

static const gchar *
get_attribute_list (CheckcopyFileHandler *fhandler)
{
  return G_FILE_ATTRIBUTE_STANDARD_SIZE;
}

/*******************/
/*- public methods-*/
/*******************/

guint
checkcopy_planner_get_num_files (CheckcopyPlanner * planner)
{
  CheckcopyPlannerPrivate *priv = GET_PRIVATE(planner);

  return priv->num_files;
}

CheckcopyPlanner*
checkcopy_planner_new (ProgressDialog * progress_dialog)
{
  return g_object_new (CHECKCOPY_TYPE_PLANNER, "progress-dialog", progress_dialog, NULL);
}
