/* $Id: error.h 50 2009-02-16 07:36:08Z squisher $ */
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

gboolean error_init();
void error_add_dialog (ProgressDialog *progress);
void show_error (char *fmt, ...);
void thread_show_error (char *fmt, ...);
gboolean error_has_occurred ();

#endif /*  __ERROR_H__ */