/* $Id: thread-copy.c 53 2009-02-16 09:39:17Z squisher $ */
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
#include <glib/gstdio.h>
#include <string.h>

#include "thread-copy.h"
#include "ring-buffer.h"
#include "error.h"

void
thread_copy (ThreadCopyParams *params)
{
  int i;

  gdk_threads_enter ();
  progress_dialog_set_status_with_text (params->progress, PROGRESS_DIALOG_STATUS_RUNNING, "Copying...");
  gdk_threads_leave ();

  params->wu = ring_buffer_get_producer_buffer ();

  g_return_if_fail (params->wu != NULL);

  for (i=1; i<params->argc; i++) {
    gchar *dir, *base;
    int last = strlen (params->argv[i]);

    if (params->argv[i][last-1] == G_DIR_SEPARATOR)
      params->argv[i][last-1] = '\0';

    dir = g_path_get_dirname (params->argv[i]);
    base = g_path_get_basename (params->argv[i]);

    copy_file (dir, base, FALSE, params);

    g_free (dir);
    g_free (base);
  }

  gdk_threads_enter ();
  progress_dialog_set_status_with_text (params->progress, PROGRESS_DIALOG_STATUS_COMPLETED, "Done!");
  gdk_threads_leave ();
  params->wu->quit = TRUE;
  params->wu->n = 0;
  ring_buffer_put ();
  g_free (params);
}


void 
copy_dir (const gchar *basepath, const gchar *path, gboolean md5_open, ThreadCopyParams *params)
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
    g_free (full_path);
    return;
  }


  out_path = g_build_filename (params->dest, path, NULL);
  if (stat (out_path, &st) == -1) {
    g_debug ("Creating %s", out_path);
    if (g_mkdir (out_path, 0777) == -1) {
      thread_show_error ("Failed to create %s", out_path);
      g_free (full_path);
      g_free (out_path);
      return;
    }
  }
  
  g_free (out_path);

  const gchar *fn;
  while ((fn = g_dir_read_name (dir)) && !error_has_occurred ()) {
    gchar *path_ext = g_build_filename (path, fn, NULL);
    md5_open = copy_file (basepath, path_ext, md5_open, params);
    g_free (path_ext);
  }

  params->wu->close = TRUE;

  g_free (full_path);
  g_dir_close (dir);
}  

void 
perform_copy (FILE *fin, FILE *fout, ThreadCopyParams *params)
{
  workunit *wu_new, *wu;
  size_t n,m;

  wu_new = params->wu;

  while (!error_has_occurred ()) {
    wu = wu_new;
    n = m = 0;

    wu->n = n = fread (wu->buf, 1, BUF_SIZE, fin);
    if (n == 0) {
      if (feof (fin))
        return;
      if (ferror (fin)) {
        thread_show_error ("An error occurred while reading!");
        return;
      }
    }

    //g_debug ("put a buffer");
    wu_new = ring_buffer_put ();
    gdk_threads_enter ();
    progress_dialog_add_size (params->progress, wu->n);
    gdk_threads_leave ();

    while (n > 0) {
      m = fwrite (wu->buf + m, 1, n , fout);
      if (m == 0) {
        if (feof (fout)) {
          thread_show_error ("EOF occurred while writing");
          return ;
        }
        if (ferror (fout)) {
          thread_show_error ("An error occurred while writing!");
          return;
        }
      }

      n -= m;
    }

  }
}

gboolean
copy_file (const gchar *basepath, const gchar *path, gboolean md5_open, ThreadCopyParams *params)
{
  struct stat st;
  gchar *fn_in;

  fn_in = g_build_filename (basepath, path, NULL);
  gdk_threads_enter ();
  progress_dialog_set_filename (params->progress, path);
  gdk_threads_leave ();
  //g_debug ("Copying %s/%s => %s", basepath, path, fn_in);

  if (g_stat (fn_in, &st) == 0) {
    if (S_ISDIR (st.st_mode)) {
      copy_dir (basepath, path, FALSE, params);
    } else {
      FILE *fin, *fout;
      gchar *fn_out, *fn_hash;

      fn_out = g_build_filename (params->dest, path, NULL);

      if (!md5_open) {
        gchar *md5base, *md5fn, *md5path;

        md5base = g_path_get_dirname (fn_out);
        md5path = g_path_get_basename (md5base);
        md5fn = g_strdup_printf ("%s.md5", md5path);
        g_free (md5path);
        md5path = g_build_filename (md5base, md5fn, NULL);

        g_free (md5fn);
        g_free (md5base);
        md5_open = TRUE;
        params->wu->open_md5 = md5path;
        g_debug ("copy->thread open new md5 file at %s", md5path);
      }

      fin = fopen (fn_in, "r");
      if (fin == NULL) {
        thread_show_error ("Error opening %s for reading", fn_in);
        g_free (fn_in);
        g_free (fn_out);
        return FALSE;
      }

      fout = fopen (fn_out, "w");
      if (fout == NULL) {
        thread_show_error ("Error opening %s for writing", fn_out);
        g_free (fn_in);
        g_free (fn_out);
        return FALSE;
      }

      g_debug ("Copying from \n%s \t to \n%s \t...", fn_in, fn_out);

      perform_copy (fin, fout, params);

      fn_hash = g_path_get_basename (path);
      params->wu->write_hash = fn_hash;
      params->wu->n = 0;
      //g_debug ("put a buffer to write hash");
      params->wu = ring_buffer_put ();

      fclose (fin);
      fclose (fout);
      g_free (fn_out);
    }
  } else {
    thread_show_error ("Could not stat %s!", path);
    return FALSE;
  }

  g_free (fn_in);

  return md5_open;
}
