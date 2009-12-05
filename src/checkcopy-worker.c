/*
 * Copyright (c) 2009     David Mohr <david@mcbf.net>
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

#include "checkcopy-worker.h"
#include "checkcopy-traversal.h"
#include "checkcopy-planner.h"
#include "checkcopy-processor.h"
#include "checkcopy-file-info.h"
#include "checkcopy-file-list.h"

void
checkcopy_worker (CheckcopyWorkerParams * params)
{
  GAsyncQueue *ext_q;
  GQueue *int_q;

  ProgressDialog * progress_dialog;
  CheckcopyPlanner *planner;
  CheckcopyProcessor *proc;
  CheckcopyFileList * list;

  gboolean verify_only;

  verify_only = g_file_has_uri_scheme (params->dest, "verify");

  list = checkcopy_file_list_get_instance ();
  g_object_set (G_OBJECT (list), "verify-only", verify_only, NULL); 

  progress_dialog = params->progress_dialog;
  planner = checkcopy_planner_new (progress_dialog);
  proc = checkcopy_processor_new (progress_dialog, params->dest);

  int_q = g_queue_new ();

  ext_q = params->queue;
  
  g_async_queue_ref (ext_q);

  while (TRUE) {
    GFile *file;

    /* Collect everything from the external queue to calculate the size.
     * Since the files are not processed yet, stick them into the internal queue.
     * 
     * At this point the internal queue is empty, so we can block once for getting the first item
     * from the external queue
    */
    file = g_async_queue_pop (ext_q);

    progress_dialog_thread_set_status (progress_dialog, PROGRESS_DIALOG_STATUS_CALCULATING_SIZE);

    do {
      g_queue_push_tail (int_q, file);
      checkcopy_traverse (file, CHECKCOPY_FILE_HANDLER (planner));
    } while ((file = g_async_queue_try_pop (ext_q)) != NULL);

#ifdef DEBUG
    {
    gchar *size_str;
    guint64 total_size = 0;

    g_object_get (planner, "total-size", &total_size, NULL);
    size_str = g_format_size_for_display (total_size);

    DBG ("Total size is now %s", size_str);
    g_free (size_str);
    }
#endif

    progress_dialog_thread_set_status (progress_dialog, PROGRESS_DIALOG_STATUS_COPYING);

    /* Now process the internal queue */
    while ((file = g_queue_pop_head (int_q)) != NULL) {
      checkcopy_traverse (file, CHECKCOPY_FILE_HANDLER (proc));
      g_object_unref (file);
    }

    checkcopy_file_list_write_checksum (list, params->dest);
    progress_dialog_thread_set_status (progress_dialog, PROGRESS_DIALOG_STATUS_COMPLETED);

    DBG ("Waiting for more things to do");
  }

  /* we won't ever get here but just in case
   * we change the loop condition later */

  g_async_queue_unref (ext_q);
  g_queue_foreach (int_q, (GFunc) g_object_unref, NULL);
  g_queue_free (int_q);

  g_object_unref (planner);
  g_object_unref (proc);
  g_object_unref (list);

  g_object_unref (params->dest);
  g_free (params);
}
