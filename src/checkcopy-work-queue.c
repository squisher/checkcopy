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

#include "checkcopy-work-queue.h"

typedef struct {
  GMutex * mutex;
  GCond * cond;

  GList * head;
  GList * tail;
} CheckcopyWorkQueue;


static CheckcopyWorkQueue * queue = NULL;


void
checkcopy_work_queue_init (void)
{
  queue = g_new0 (CheckcopyWorkQueue, 1);
}

void
checkcopy_work_queue_free (void)
{
  GList * files;
  g_assert (queue != NULL);

  for (files = queue->head; files != NULL; files = files->next) {
    g_free (files->data);
  }

  g_list_free (queue->head);

  g_free (queue);
}

void
checkcopy_work_queue_push (GFile *file)
{
  g_assert (queue != NULL);

}

GFile *
checkcopy_work_queue_pop (void)
{
  GFile * file = NULL;

  g_assert (queue != NULL);

  if (queue->head == queue->tail) {
    file = queue->head->data;
    queue->head = queue->tail = NULL;
  } else if (queue->head != NULL && queue->tail != NULL) {
    file = queue->head->data;
    queue->head = g_list_delete_link (queue->head, queue->tail);
  }

  return file;
}
