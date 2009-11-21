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

#ifndef __CHECKCOPY_DETAILS_WINDOW__
#define __CHECKCOPY_DETAILS_WINDOW__

#include <glib-object.h>

G_BEGIN_DECLS

#define CHECKCOPY_TYPE_DETAILS_WINDOW checkcopy_details_window_get_type()

#define CHECKCOPY_DETAILS_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHECKCOPY_TYPE_DETAILS_WINDOW, CheckcopyDetailsWindow))

#define CHECKCOPY_DETAILS_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), CHECKCOPY_TYPE_DETAILS_WINDOW, CheckcopyDetailsWindowClass))

#define CHECKCOPY_IS_DETAILS_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHECKCOPY_TYPE_DETAILS_WINDOW))

#define CHECKCOPY_IS_DETAILS_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), CHECKCOPY_TYPE_DETAILS_WINDOW))

#define CHECKCOPY_DETAILS_WINDOW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHECKCOPY_TYPE_DETAILS_WINDOW, CheckcopyDetailsWindowClass))

typedef struct {
  GtkWindow parent;
} CheckcopyDetailsWindow;

typedef struct {
  GtkWindowClass parent_class;
} CheckcopyDetailsWindowClass;

GType checkcopy_details_window_get_type (void);

CheckcopyDetailsWindow* checkcopy_details_window_new (void);

G_END_DECLS

#endif /* __CHECKCOPY_DETAILS_WINDOW__ */
