/* $Id: md5copy.c 39 2008-07-20 19:18:04Z squisher $ */
/*
 *  Copyright (c) 2008 David Mohr <david@mcbf.net>
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
#include <glib/gstdio.h>
#include <stdlib.h>
#include <mhash.h>
#include <stdarg.h>

#include "progress-dialog.h"
#include "global.h"
#include "ring-buffer.h"

#define PROG_NAME "md5copy"
#define VERSION "0.1"


typedef struct {
  int argc;
  char **argv;
  ProgressDialog *progress;
} ThreadCopyParams;


/* 
 * prototypes 
 */

/* main thread */
guint64 get_dir_size (const gchar *path);
guint64 get_size (const gchar *path);
gchar * ask_for_destination ();
void show_error (char *fmt, ...);
void show_verror (char *fmt, va_list ap);
void thread_show_error (char *fmt, ...);

/* thread copy */
void thread_copy (ThreadCopyParams *params);
void perform_copy (FILE *fin, FILE *fout, ProgressDialog *progress);
gboolean copy_file (const gchar *basepath, const gchar *path, gboolean md5_open, ProgressDialog *progress);
void copy_dir (const gchar *basepath, const gchar *path, gboolean md5_open, ProgressDialog *progress);

/* thread hash */
void print_digest (FILE *fp, char *fn_hash, unsigned char *digest);
void thread_hash ();


/*
 * globals 
 */

gchar *dest = NULL;
MHASH master_hash;
workunit *wu;
GtkWidget *progress_dialog = NULL;


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

void 
show_error (char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  show_verror (fmt, ap);
  va_end (ap);
}

void 
show_verror (char *fmt, va_list ap) 
{
  gchar *msg;

  msg = g_strdup_vprintf (fmt, ap);
  if (progress_dialog == NULL) {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                     msg);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  } else {
    progress_dialog_set_status_with_text (PROGRESS_DIALOG (progress_dialog), PROGRESS_DIALOG_STATUS_FAILED, msg);
  }
  g_free (msg);
}

void 
thread_show_error (char *fmt, ...)
{
  va_list ap;

  wu->quit = TRUE;
  wu = ring_buffer_put ();

  va_start (ap, fmt);
  gdk_threads_enter ();
  show_verror (fmt, ap);
  gdk_threads_leave ();
  va_end (ap);

  g_thread_exit(NULL);
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



void
thread_copy (ThreadCopyParams *params)
{
  int i;

  gdk_threads_enter ();
  progress_dialog_set_status_with_text (params->progress, PROGRESS_DIALOG_STATUS_RUNNING, "Copying...");
  gdk_threads_leave ();

  for (i=1; i<params->argc; i++) {
    gchar *dir, *base;
    int last = strlen (params->argv[i]);

    if (params->argv[i][last-1] == G_DIR_SEPARATOR)
      params->argv[i][last-1] = '\0';

    dir = g_path_get_dirname (params->argv[i]);
    base = g_path_get_basename (params->argv[i]);

    copy_file (dir, base, FALSE, params->progress);

    g_free (dir);
    g_free (base);
  }

  gdk_threads_enter ();
  progress_dialog_set_status_with_text (params->progress, PROGRESS_DIALOG_STATUS_COMPLETED, "Done!");
  gdk_threads_leave ();
  wu->quit = TRUE;
  wu->n = 0;
  wu = ring_buffer_put ();
  g_free (params);
}


void 
copy_dir (const gchar *basepath, const gchar *path, gboolean md5_open, ProgressDialog *progress)
{
  GDir *dir;
  GError *error = NULL;
  gchar *full_path;
  gchar *out_path;
  struct stat st;

  //if (fp == NULL)
  full_path = g_build_filename (basepath, path, NULL);
  //g_debug ("Opening dir %s/%s => %s", basepath, path, full_path);
  //g_debug ("Opening dir %s", full_path);

  dir = g_dir_open (full_path, 0, &error);
  if (dir == NULL) {
    thread_show_error (error->message);
    g_error_free (error);
  }


  out_path = g_build_filename (dest, path, NULL);
  if (stat (out_path, &st) == -1) {
    g_debug ("Creating %s", out_path);
    if (g_mkdir (out_path, 0777) == -1)
      thread_show_error ("Failed to create %s", out_path);
  } else {
    g_debug ("%s exists, skipping", out_path);
  }
  
  g_free (out_path);

  const gchar *fn;
  while ((fn = g_dir_read_name (dir))) {
    gchar *path_ext = g_build_filename (path, fn, NULL);
    md5_open = copy_file (basepath, path_ext, md5_open, progress);
    g_free (path_ext);
  }

  wu->close = TRUE;

  g_free (full_path);
  g_dir_close (dir);
}  

void 
perform_copy (FILE *fin, FILE *fout, ProgressDialog *progress)
{
  workunit *wu_new;
  size_t n,m;

  wu_new = wu;

  while (1) {
    wu = wu_new;
    n = m = 0;

    wu->n = n = fread (wu->buf, 1, BUF_SIZE, fin);
    if (n == 0) {
      if (feof (fin))
        return;
      if (ferror (fin))
        thread_show_error ("An error occurred while reading!");
    }

    //g_debug ("put a buffer");
    wu_new = ring_buffer_put ();
    gdk_threads_enter ();
    progress_dialog_add_size (progress, wu->n);
    gdk_threads_leave ();

    while (n > 0) {
      m = fwrite (wu->buf + m, 1, n , fout);
      if (m == 0) {
        if (feof (fout))
          thread_show_error ("EOF occurred while writing");
        if (ferror (fout))
          thread_show_error ("An error occurred while writing!");
      }

      n -= m;
    }

  }
}

gboolean
copy_file (const gchar *basepath, const gchar *path, gboolean md5_open, ProgressDialog *progress)
{
  struct stat st;
  gchar *fn_in;

  fn_in = g_build_filename (basepath, path, NULL);
  gdk_threads_enter ();
  progress_dialog_set_filename (progress, path);
  gdk_threads_leave ();
  //g_debug ("Copying %s/%s => %s", basepath, path, fn_in);

  if (g_stat (fn_in, &st) == 0) {
    if (S_ISDIR (st.st_mode)) {
      copy_dir (basepath, path, FALSE, progress);
    } else {
      FILE *fin, *fout;
      gchar *fn_out, *fn_hash;

      fn_out = g_build_filename (dest, path, NULL);

      if (!md5_open) {
        gchar *md5base, *md5fn, *md5path;

        md5base = g_path_get_dirname (fn_out);
        md5path = g_path_get_basename (md5base);
        md5fn = g_strdup_printf ("%s.md5", md5path);
        g_free (md5path);
        md5path = g_build_filename (md5base, md5fn, NULL);

        g_free (md5fn);
        g_free (md5base);
        wu->fn = md5path;
        wu->open_md5 = md5_open = TRUE;
      }

      fin = fopen (fn_in, "r");
      if (fin == NULL)
        thread_show_error ("Error opening %s for reading", fn_in);
      fout = fopen (fn_out, "w");
      if (fout == NULL)
        thread_show_error ("Error opening %s for writing", fn_out);

      g_debug ("Copying from \n%s \t to \n%s \t...", fn_in, fn_out);

      perform_copy (fin, fout, progress);

      fn_hash = g_path_get_basename (path);
      wu->write_hash = TRUE;
      wu->n = 0;
      wu->fn = fn_hash;
      //g_debug ("put a buffer to write hash");
      wu = ring_buffer_put ();

      fclose (fin);
      fclose (fout);
      g_free (fn_out);
    }
  } else {
    thread_show_error ("Could not stat %s!", path);
  }

  g_free (fn_in);

  return md5_open;
}



void
print_digest (FILE *fp, char *fn_hash, unsigned char *digest)
{
  int i;

  //g_debug ("blocksize = %d", mhash_get_block_size (MHASH_MD5));
  for (i=0; i<mhash_get_block_size (MHASH_MD5); i++) {
    fprintf (fp, "%.2x", digest[i]);
  }
  fprintf (fp, " *%s\n", fn_hash);
}

void
thread_hash ()
{
  workunit *wu_cur;
  FILE *fp;
  MHASH hash;
  unsigned char digest[16]; // FIXME: 16 is only good for md5

  hash = mhash_cp (master_hash);

  while (1) {
    //g_debug ("HASH: waiting for buffer");
    wu_cur = ring_buffer_get ();
    if (wu_cur == NULL)
      thread_show_error ("Threads out of sync!");
    //g_debug ("HASH: got a buffer");

    if (wu_cur->close) {
      fclose (fp);
      wu_cur->close = FALSE;
      //g_debug ("HASH: closing md5 file");
    }

    if (wu_cur->open_md5) {
      //g_debug ("md5 file = %s", wu_cur->fn);
      fp = fopen (wu_cur->fn, "w");
      if (fp == NULL)
        thread_show_error ("Error opening %s for writing the hash!", wu_cur->fn);
      wu_cur->open_md5 = FALSE;
      g_free (wu_cur->fn);
    }

    if (wu_cur->n)
      mhash (hash, wu_cur->buf, wu_cur->n);

    if (wu_cur->write_hash) {
      //g_debug ("HASH: writing hash for %s", wu_cur->fn);
      mhash_deinit (hash, digest);
      print_digest (fp, wu_cur->fn, digest);
      wu_cur->write_hash = FALSE;
      g_free (wu_cur->fn);
      hash = mhash_cp (master_hash);
    }

    if (wu_cur->quit)
      break;
  }
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

  if (show_version) {
    printf ("%s %s\n"
            "Copyright (C) 2008 David Mohr\n"
            "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n"
            "This is free software: you are free to change and redistribute it.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n", PROG_NAME, VERSION);

    return 0;
  }

  if (argc < 2) {
    show_error ("Usage: %s %s", argv[0], arg_description);

    return 1;
  }


  /* get a destination */
  //dest = "tmp";
  if (dest == NULL)
    dest = ask_for_destination();
  
  /* user aborted */
  if (dest == NULL)
    exit (0);

  g_debug ("Destination is %s", dest);


  /* calculate the size */
  guint64 total_size = 0;

  for (i=1; i<argc; i++) {
    guint64 size = 0;
    size = get_size (argv[i]);
    total_size += size;
  }

  g_debug ("Size = %llu\n", total_size);


  /* initialize mhash */
  master_hash = mhash_init (MHASH_MD5);
  if (master_hash == MHASH_FAILED)
    show_error ("Could not initialize mhash library!");


  /*
  gchar *md5fn, *md5path;
  FILE *md5fp;

  md5path = g_path_get_basename (dest);
  md5fn = g_strdup_printf ("%s.md5", dest);
  g_free (md5path);
  md5path = g_build_filename (dest, md5fn, NULL);

  md5fp = fopen (md5path, "w");
  if (md5fp == NULL)
    show_error ("Could not create %s", md5path);
  g_free (md5fn);
  g_free (md5path);
  */


  /* show the progress dialog */
  progress_dialog = progress_dialog_new (total_size);
  gtk_widget_show (progress_dialog);

  /* create thread to do the copying */
  wu = ring_buffer_init ();

  ThreadCopyParams *params;
  params = g_new0 (ThreadCopyParams, 1);
  params->argc = argc;
  params->argv = argv;
  params->progress = PROGRESS_DIALOG (progress_dialog);

  g_thread_create ((GThreadFunc) thread_copy, params, FALSE, NULL);

  g_thread_create ((GThreadFunc) thread_hash, NULL, FALSE, NULL);

  /* transfer over to gtk */
  gtk_main ();

  
  gdk_threads_leave ();

  //fclose (md5fp);
  mhash_deinit (master_hash, NULL);

#ifdef STATS
  g_message ("%d producer waits, \t %d consumer waits", ring_buffer_prod_waits (), ring_buffer_cons_waits ());
#endif
    
  return 0;
}
