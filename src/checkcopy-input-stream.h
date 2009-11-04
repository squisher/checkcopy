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

#ifndef __CHECKCOPY_INPUT_STREAM__
#define __CHECKCOPY_INPUT_STREAM__

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define CHECKCOPY_TYPE_INPUT_STREAM checkcopy_input_stream_get_type()

#define CHECKCOPY_INPUT_STREAM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CHECKCOPY_TYPE_INPUT_STREAM, CheckcopyInputStream))

#define CHECKCOPY_INPUT_STREAM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), CHECKCOPY_TYPE_INPUT_STREAM, CheckcopyInputStreamClass))

#define CHECKCOPY_IS_INPUT_STREAM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CHECKCOPY_TYPE_INPUT_STREAM))

#define CHECKCOPY_IS_INPUT_STREAM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), CHECKCOPY_TYPE_INPUT_STREAM))

#define CHECKCOPY_INPUT_STREAM_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), CHECKCOPY_TYPE_INPUT_STREAM, CheckcopyInputStreamClass))

typedef struct {
  GFilterInputStream parent;
} CheckcopyInputStream;

typedef struct {
  GFilterInputStreamClass parent_class;
} CheckcopyInputStreamClass;

GType checkcopy_input_stream_get_type (void);

CheckcopyInputStream* checkcopy_input_stream_new (void);

G_END_DECLS

#endif /* __CHECKCOPY_INPUT_STREAM__ */
