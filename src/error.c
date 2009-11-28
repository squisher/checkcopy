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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libxfce4util/libxfce4util.h>

#include <gtk/gtk.h>
#include <glib.h>

#include "progress-dialog.h"
#include "error.h"
#include "checkcopy-cancel.h"

/* globals */
ProgressDialog *progress = NULL;
gboolean error_occurred;


/* private prototypes  */

void show_verror (gchar * file, gint line, char *fmt, va_list ap);


/* private functions */

void 
show_verror (gchar * file, gint line, char *fmt, va_list ap) 
{
  gchar *msg;

  error_occurred = TRUE;


  msg = g_strdup_vprintf (fmt, ap);

  if (file) {
    g_warning ("%s:%d  %s", file, line, msg);
  } else {
    g_warning ("%s", msg);
  }

  if (progress == NULL) {
    GtkWidget *dialog;
    gint r;

    dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK_CANCEL,
                                     "%s", msg);
    r = gtk_dialog_run (GTK_DIALOG (dialog));

    switch (r) {
      case GTK_RESPONSE_OK:
        break;
      case GTK_RESPONSE_CANCEL:
        {
          GCancellable *cancel;

          cancel = checkcopy_get_cancellable ();
          g_cancellable_cancel (cancel);
        }
        break;
      default:
        g_critical ("Invalid dialog response");
    }
    gtk_widget_destroy (dialog);
  } else {
    // FIXME
    //progress_dialog_set_status_with_text (progress, PROGRESS_DIALOG_STATUS_FAILED, msg);
  }
  g_free (msg);
}


/* public functions */

void 
show_error (char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  show_verror (NULL, 0, fmt, ap);
  va_end (ap);
}

void 
thread_show_error_full (gchar * file, gint line, char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  gdk_threads_enter ();
  show_verror (file, line, fmt, ap);
  gdk_threads_leave ();
  va_end (ap);
}

void 
thread_show_gerror_full (gchar * file, gint line, GError *error)
{
  g_assert (error != NULL);

  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    return;

  thread_show_error_full (file, line, "%s", error->message);
}

gboolean
error_has_occurred (void)
{
  gboolean ret;
  ret = error_occurred;

  return ret;
}


GQuark
checkcopy_error_quark (void)
{
  return g_quark_from_static_string ("checkcopy-error-quark");
}


void
error_add_dialog (ProgressDialog *progress_dialog)
{
  progress = progress_dialog;
}

gboolean
error_init (void)
{
  error_occurred = FALSE;

  return TRUE;
}

