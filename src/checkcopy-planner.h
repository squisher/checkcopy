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

#ifndef __CHECKCOPY_PLANNER__
#define __CHECKCOPY_PLANNER__

#include <glib-object.h>

#include "progress-dialog.h"

G_BEGIN_DECLS

#define CHECKCOPY_TYPE_PLANNER checkcopy_planner_get_type()

#define CHECKCOPY_PLANNER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHECKCOPY_TYPE_PLANNER, CheckcopyPlanner))

#define CHECKCOPY_PLANNER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), CHECKCOPY_TYPE_PLANNER, CheckcopyPlannerClass))

#define CHECKCOPY_IS_PLANNER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHECKCOPY_TYPE_PLANNER))

#define CHECKCOPY_IS_PLANNER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), CHECKCOPY_TYPE_PLANNER))

#define CHECKCOPY_PLANNER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHECKCOPY_TYPE_PLANNER, CheckcopyPlannerClass))

typedef struct {
  GObject parent;
} CheckcopyPlanner;

typedef struct {
  GObjectClass parent_class;
} CheckcopyPlannerClass;

GType checkcopy_planner_get_type (void);

CheckcopyPlanner* checkcopy_planner_new (ProgressDialog * progress_dialog);

G_END_DECLS

#endif /* __CHECKCOPY_PLANNER__ */
