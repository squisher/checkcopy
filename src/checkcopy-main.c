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

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <libxfce4util/libxfce4util.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include "checkcopy-cancel.h"
#include "checkcopy-traversal.h"
#include "checkcopy-processor.h"
#include "checkcopy-worker.h"

#include "progress-dialog.h"
#include "error.h"

/*
 * prototypes
 */

GFile * ask_for_destination ();
#if 0
void debug_log_handler (const gchar *domain, GLogLevelFlags level, const gchar *msg, gpointer data);
#endif


/*
 * globals
 */

static gchar *dest = NULL;
static gchar *default_dest = NULL;
static gboolean show_version = FALSE;

static GOptionEntry optionentries[] = {
  { "destination", 'd', G_OPTION_FLAG_FILENAME, G_OPTION_ARG_STRING, &dest,
    "Specifies the destination of the copy operation, or verify://", "SOME_DIR"},
  { "default-destination", 'D', G_OPTION_FLAG_FILENAME, G_OPTION_ARG_STRING, &default_dest,
    "Specifies the default destination of the copy operation", "SOME_DIR"},
  { "version", 'V', G_OPTION_FLAG_NO_ARG , G_OPTION_ARG_NONE, &show_version,
    "Display program version and exit", NULL },
  { NULL, ' ', 0, 0, NULL, NULL, NULL },
};



/*
 * implementation
 */

#if 0
/* suppreses debug messages when DEBUG is not enabled, see main () */
void debug_log_handler (const gchar *domain, GLogLevelFlags level, const gchar *msg, gpointer data)
{
}
#endif

/* returns a string that must be freed */
GFile *
ask_for_destination (void)
{
  GtkWidget *filechooser;
  gint res;
  GFile *folder = NULL;
  GFile *def_folder;

  filechooser = gtk_file_chooser_dialog_new ("Destination", NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                             GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
  gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (filechooser), FALSE);

  if (default_dest) {
    gchar *uri;

    def_folder = g_file_new_for_commandline_arg (default_dest);
    uri = g_file_get_uri (def_folder);

    DBG ("Setting default destination to %s", uri);
    gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (filechooser), uri);
  }

  res = gtk_dialog_run (GTK_DIALOG (filechooser));

  switch (res) {
    case GTK_RESPONSE_ACCEPT:
      folder = gtk_file_chooser_get_current_folder_file (GTK_FILE_CHOOSER (filechooser));
      break;
  }
  gtk_widget_destroy (filechooser);

  return folder;
}



int
main (int argc, char *argv[])
{
  GError * error = NULL;
  gchar * arg_description = "FILE/DIR {FILE/DIR2 ...}";
  ProgressDialog * progress_dialog = NULL;
#ifdef STATS
  gchar *stats;
#endif
  GFile * dest_folder;
  GAsyncQueue * queue;
  CheckcopyWorkerParams *worker_params;
  int i;
  gboolean verify_only = FALSE;

#if 0
#if DEBUG > 0
  /* Not handlingc critical as fatal until gdk bug is resolved
  g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);
  */
#else
  g_log_set_handler (NULL, G_LOG_LEVEL_DEBUG, debug_log_handler, NULL);
#endif
#endif

  g_set_application_name (PACKAGE_NAME);


  g_thread_init (NULL);
  gdk_threads_init ();
  gdk_threads_enter ();
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  if (!gtk_init_with_args (&argc, &argv, arg_description, optionentries, NULL, &error)) {
    if (error != NULL) {
      g_print ("%s: %s\nTry %s --help to see a full list of available command line options.\n", PACKAGE_NAME, error->message, argv[0]);
      g_error_free (error);
      return 1;
    }
  }

  checkcopy_cancel_init ();

  if (show_version) {
    printf ("%s %s\n"
            "Copyright (C) 2008-2009 David Mohr\n"
            "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n"
            "This is free software: you are free to change and redistribute it.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n", PACKAGE_NAME, VERSION);

    return 0;
  }

  if (argc < 2) {
    show_error ("Usage: %s %s", argv[0], arg_description);

    return 1;
  }

  /* get a destination, if none was specified on the command line */
  if (dest == NULL) {
    dest_folder = ask_for_destination();
  } else {
    dest_folder = g_file_new_for_commandline_arg (dest);

    if (g_file_has_uri_scheme (dest_folder, "verify")) {
      verify_only = TRUE;
      g_message ("Verification-only mode");
    }
  }

  /* user aborted */
  if (dest_folder == NULL)
    goto exit;

#ifdef DEBUG
  dest = g_file_get_uri (dest_folder);

  DBG ("Destination is %s\n", dest);

  g_free (dest);
#endif

  /* show the progress dialog */
  progress_dialog = progress_dialog_new (verify_only);
  gtk_widget_show (GTK_WIDGET (progress_dialog));

  /* prepare the worker thread */
  worker_params = g_new0 (CheckcopyWorkerParams, 1);

  queue = g_async_queue_new_full ((GDestroyNotify) g_object_unref);
  worker_params->queue = queue;
  worker_params->dest = dest_folder;
  worker_params->progress_dialog = progress_dialog;

  for (i=1; i<argc; i++) {
    g_async_queue_push (queue, g_file_new_for_commandline_arg (argv[i]));
  }

  g_thread_create ((GThreadFunc) checkcopy_worker, worker_params, FALSE, NULL);


  /* transfer over to gtk */
  gtk_main ();

exit:
  g_async_queue_unref (queue);

  /* finalization */
  gdk_threads_leave ();

  return EXIT_SUCCESS;
}
