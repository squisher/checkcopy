/* $Id: thread-copy.h 50 2009-02-16 07:36:08Z squisher $ */
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

#ifndef __THREAD_COPY_H__
#define __THREAD_COPY_H__

#include "global.h"
#include "progress-dialog.h"

typedef struct {
  int argc;
  char **argv;
  gchar *dest;
  ProgressDialog *progress;
  workunit *wu;
} ThreadCopyParams;

void thread_copy (ThreadCopyParams *params);
void perform_copy (FILE *fin, FILE *fout, ThreadCopyParams *params);
gboolean copy_file (const gchar *basepath, const gchar *path, gboolean md5_open, ThreadCopyParams *params);
void copy_dir (const gchar *basepath, const gchar *path, gboolean md5_open, ThreadCopyParams *params);

#endif /*  __THREAD_COPY_H__ */
