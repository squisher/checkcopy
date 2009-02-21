/* $Id: md5copy.c 53 2009-02-16 09:39:17Z squisher $ */
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

#include <mhash.h>
#include "mhash-fix.h"

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "progress-dialog.h"
#include "global.h"
#include "ring-buffer.h"
#include "thread-copy.h"
#include "thread-hash.h"
#include "error.h"

#define PROG_NAME "md5copy"
#define VERSION "0.2"


/* 
 * prototypes 
 */

guint64 get_dir_size (const gchar *path);
guint64 get_size (const gchar *path);
gchar * ask_for_destination ();


/*
 * globals 
 */

gchar *dest = NULL;


/* 
 * implementation 
 */

/* suppreses debug messages when DEBUG is not enabled, see main () */
void debug_log_handler (const gchar *domain, GLogLevelFlags level, const gchar *msg, gpointer data)
{
}

/* returns a string that must be freed */
gchar *
ask_for_destination ()
{
  GtkWidget *filechooser;
  gint res;
  gchar *ret;

  filechooser = gtk_file_chooser_dialog_new ("Destination", NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                             GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
  res = gtk_dialog_run (GTK_DIALOG (filechooser));

  switch (res) {
    case GTK_RESPONSE_ACCEPT:
    ret = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filechooser));
      break;
    default:
      ret = NULL;
  }
  gtk_widget_destroy (filechooser);

  return ret;
}

guint64 
get_dir_size (const gchar *path)
{
  GDir *dir;
  GError *error = NULL;
  guint64 size = 0;

  dir = g_dir_open (path, 0, &error);
  if (dir == NULL) {
    show_error (error->message);
    g_error_free (error);
    exit (1);
  }

  g_debug ("Opening dir %s", path);

  const gchar *fn;
  while ((fn = g_dir_read_name (dir))) {
    gchar *full_fn = g_build_filename (path, fn, NULL);
    size += get_size (full_fn);
    //g_debug ("After %s the dir size is %llu", full_fn, size);
    g_free (full_fn);
  }

  g_dir_close (dir);

  return size;
}  

guint64 
get_size (const gchar *path)
{
  struct stat st;
  guint64 size;

  //g_debug ("Getting size of %s", path);

  if (g_stat (path, &st) == 0) {
    if (S_ISDIR (st.st_mode)) {
      size = get_dir_size (path);
    } else {
      size = st.st_size;
    }
  } else {
    show_error ("Could not stat %s!", path);
  }
  //g_debug ("%s has size %lu", path, size);
  return size;
}




gboolean show_version = FALSE;

static GOptionEntry optionentries[] = {
  { "destination", 'd', G_OPTION_FLAG_FILENAME, G_OPTION_ARG_STRING, &dest, 
    "Specifies the destination of the copy operation", "SOME_DIR"},
  { "version", 'V', G_OPTION_FLAG_NO_ARG , G_OPTION_ARG_NONE, &show_version, 
    "Display program version and exit", NULL },
  { NULL },
};

int 
main (int argc, char *argv[])
{
  GError *error = NULL;
  int i;
  gchar *arg_description = "FILE/DIR {FILE/DIR2 ...}";
  MHASH master_hash;
  GtkWidget *progress_dialog = NULL;
#ifdef STATS
  gchar *stats;
#endif
  int len;
  gchar *display_dest;

#if DEBUG > 0
  g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);
#else
  g_log_set_handler (NULL, G_LOG_LEVEL_DEBUG, debug_log_handler, NULL);
#endif
  
  g_set_application_name (PROG_NAME);

  if (!g_thread_supported ())
    g_thread_init (NULL);

  gdk_threads_init ();
  gdk_threads_enter ();


  if (!gtk_init_with_args (&argc, &argv, arg_description, optionentries, NULL, &error)) {
    if (error != NULL) {
      g_print ("%s: %s\nTry %s --help to see a full list of available command line options.\n", PROG_NAME, error->message, argv[0]);
      g_error_free (error);
      return 1;
    }
  }

  error_init();

  if (show_version) {
    printf ("%s %s\n"
            "Copyright (C) 2008-2009 David Mohr\n"
            "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n"
            "This is free software: you are free to change and redistribute it.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n", PROG_NAME, VERSION);

    return 0;
  }

  if (argc < 2) {
    show_error ("Usage: %s %s", argv[0], arg_description);

    return 1;
  }


  /* get a destination, if none was specified on the command line */
  if (dest == NULL)
    dest = ask_for_destination();
  else
    dest = g_strdup (dest);
  
  /* user aborted */
  if (dest == NULL)
    exit (0);

  g_debug ("Destination is %s", dest);


  /* show the progress dialog */
  progress_dialog = progress_dialog_new ();
  gtk_widget_show (progress_dialog);

  progress_dialog_set_status (PROGRESS_DIALOG (progress_dialog), PROGRESS_DIALOG_STATUS_CALCULATING_SIZE);

  error_add_dialog (PROGRESS_DIALOG (progress_dialog));


  /* add the full destination, or abbreviated, to the window title */
  if ((len = strlen (dest)) > MAX_FILENAME_LEN)
    display_dest = g_strdup_printf ("%s to ...%s", PROG_NAME, dest + (len - MAX_FILENAME_LEN));
  else
    display_dest = g_strdup_printf ("%s to %s", PROG_NAME, dest);
    
  gtk_window_set_title (GTK_WINDOW (progress_dialog), display_dest);
  g_free (display_dest);


  /* Calculate the size of the whole job.
   * Warning: This might be slow if a lot of files will be copied */
  guint64 total_size = 0;

  for (i=1; i<argc; i++) {
    guint64 size = 0;
    size = get_size (argv[i]);
    total_size += size;
  }

  g_object_set (progress_dialog, "total_size", total_size, NULL);
  g_debug ("Size = %llu\n", total_size);


  /* initialize mhash */
  master_hash = mhash_init (MHASH_MD5);
  if (master_hash == MHASH_FAILED)
    show_error ("Could not initialize mhash library!");


  /* create thread to do the copying */
  ring_buffer_init ();

  ThreadCopyParams *params;
  params = g_new0 (ThreadCopyParams, 1);
  params->argc = argc;
  params->argv = argv;
  params->dest = dest;
  params->progress = PROGRESS_DIALOG (progress_dialog);

  g_thread_create ((GThreadFunc) thread_copy, params, FALSE, NULL);

  g_thread_create ((GThreadFunc) thread_hash, master_hash, FALSE, NULL);

  /* transfer over to gtk */
  gtk_main ();

  
  /* finalization */
  gdk_threads_leave ();

  g_free (dest);

  mhash_deinit (master_hash, NULL);

#ifdef STATS
  g_message ("%s", (stats = ring_buffer_get_stats ()));
  g_free (stats);
#endif

  ring_buffer_free ();

  g_object_unref (progress_dialog);
    
  return 0;
}
