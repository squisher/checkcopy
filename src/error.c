/* $Id: error.c 53 2009-02-16 09:39:17Z squisher $ */
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

#include <gtk/gtk.h>
#include <glib.h>

#include "global.h"
#include "progress-dialog.h"
#include "error.h"

/* globals */
ProgressDialog *progress = NULL;
//GMutex *mutex = NULL;
gboolean error_occurred;


/* private prototypes  */

void show_verror (char *fmt, va_list ap);


/* public functions */

gboolean
error_init ()
{
  error_occurred = FALSE;

  /*
   * accessing a boolean from several threads should be OK,
   * since it is either set or not set, there is no false
   * value that could be read.
  mutex = g_mutex_new ();

  return mutex != NULL;
  */

  return TRUE;
}

void
error_add_dialog (ProgressDialog *progress_dialog)
{
  progress = progress_dialog;
}

void 
show_error (char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  show_verror (fmt, ap);
  va_end (ap);
}

void 
thread_show_error (char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  gdk_threads_enter ();
  show_verror (fmt, ap);
  gdk_threads_leave ();
  va_end (ap);

  g_thread_exit(NULL);
}


gboolean
error_has_occurred ()
{
  gboolean ret;
//  g_mutex_lock (mutex);
  ret = error_occurred;
//  g_mutex_unlock (mutex);

  return ret;
}


/* private functions */

void 
show_verror (char *fmt, va_list ap) 
{
  gchar *msg;

//  g_mutex_lock (mutex);
  error_occurred = TRUE;
//  g_mutex_unlock (mutex);


  msg = g_strdup_vprintf (fmt, ap);

  g_warning (msg);

  if (progress == NULL) {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                     msg);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  } else {
    progress_dialog_set_status_with_text (progress, PROGRESS_DIALOG_STATUS_FAILED, msg);
  }
  g_free (msg);
}

