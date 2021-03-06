/*
 *  Copyright (c) 2008-2009 David Mohr <david@mcbf.net>
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
 */

#ifndef __ERROR_H__
#define __ERROR_H__

#include "progress-dialog.h"

#define CHECKCOPY_ERROR checkcopy_error_quark ()

enum {
  CHECKCOPY_ERROR_GENERIC,
  CHECKCOPY_ERROR_CONSUMER_TIMEOUT,
  CHECKCOPY_ERROR_INVALID_STATE_CHANGE,
};

#if 0
gboolean error_init();
#endif
void error_add_dialog (ProgressDialog *progress);
void show_error (char *fmt, ...);
void show_error_full (gchar * file, gint line, gboolean abortable, char *fmt, ...);

#define thread_show_error(fmt, args...) \
  thread_show_error_full (__FILE__, __LINE__, (fmt), ## args)
void thread_show_error_full (gchar * file, gint line, char *fmt, ...);
#define thread_show_gerror(file, error) \
  thread_show_gerror_full (__FILE__, __LINE__, (file), (error))
void thread_show_gerror_full (gchar * srcfile, gint line, GFile * file, GError *error);

#if 0
gboolean error_has_occurred ();
#endif
GQuark checkcopy_error_quark ();

#endif /*  __ERROR_H__ */
