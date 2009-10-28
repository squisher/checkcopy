/* $Id: ring-buffer.c 54 2009-02-16 10:34:59Z squisher $ */
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

#include <glib.h>
#include <stdlib.h>

#include "global.h"
#include "ring-buffer.h"
#include "settings.h"
#include "error.h"


#if DEBUG > 1
# define DEBUG_RING_BUFFER
#endif


RING_BUFFER_TYPE **buffers = NULL;
GMutex *mutex = NULL;
GCond *cond_prod = NULL;
GCond *cond_cons = NULL;
gboolean done = FALSE;
int prod, cons;

#ifdef STATS
int prod_waits, cons_waits, cons_retries;
#endif


gboolean
ring_buffer_init ()
{
  int i;
  int pointer_size = (sizeof (RING_BUFFER_TYPE *) * BUF_SLOTS);

  /* allocate in one step the buffers array (pointers + data) */
  /* FIXME: not sure how portable this is */
  buffers = (RING_BUFFER_TYPE **) g_malloc0 (pointer_size + (BUF_SLOTS * sizeof (RING_BUFFER_TYPE)));

  for (i=0; i<BUF_SLOTS; i++) {
    buffers[i] = (RING_BUFFER_TYPE *) (buffers + BUF_SLOTS) + i;
  }
  mutex = g_mutex_new ();
  cond_prod = g_cond_new ();
  cond_cons = g_cond_new ();

  prod = 0;
  cons = BUF_SLOTS - 1;

#ifdef STATS
  prod_waits = cons_waits = cons_retries = 0;
#endif
  return TRUE;
}

/* this function should only be used exactly once,
 * by the producer to get the initial buffer it
 * will be using. After that a new buffer will be
 * returned by ring_buffer_put ()
 */
RING_BUFFER_TYPE *
ring_buffer_get_producer_buffer ()
{
  static gboolean producer_active = FALSE;

  g_assert (buffers != NULL);

  if (producer_active)
    return NULL;

  producer_active = TRUE;

  return buffers[prod];
}


RING_BUFFER_TYPE *
ring_buffer_put ()
{
  int t;

  g_mutex_lock (mutex);
  t = (prod + 1) % BUF_SLOTS;
  if (t == cons) {
#ifdef STATS
    prod_waits++;
#if DEBUG_RING_BUFFER
    if (prod_waits < 10)
      g_debug ("producer wait %d", prod_waits);
#endif
#endif
    g_cond_signal (cond_prod);
    g_cond_wait (cond_cons, mutex);
  }
  prod = t;

  g_cond_signal (cond_prod);
#ifdef DEBUG_RING_BUFFER
  g_debug ("Producer sent a signal");
#endif
  g_mutex_unlock (mutex);

  return buffers[prod];
}


RING_BUFFER_TYPE *
ring_buffer_get (GError **error)
{
  int t,i;
  gboolean got_signal = TRUE;
  GTimeVal time;

  g_mutex_lock (mutex);
  t = (cons + 1) % BUF_SLOTS;
  if (t == prod) {
    g_cond_signal (cond_cons);
    for (i=0; i<MAX_CONSUMER_RETRIES; i++) {
#ifdef STATS
      cons_waits++;
# if DEBUG_RING_BUFFER
      if (cons_waits < 10)
        g_debug ("consumer wait %d", cons_waits);
# endif
#endif

      g_get_current_time (&time);
      g_time_val_add (&time, CONS_WAIT_TIME);

#ifdef DEBUG_RING_BUFFER
      g_debug ("Consumer starts to sleep to wait for a signal");
#endif
      got_signal = g_cond_timed_wait (cond_prod, mutex, &time);
      if (got_signal && t != prod) {
#ifdef DEBUG_RING_BUFFER
        g_debug ("Consumer got signal & has work");
#endif
        break;
      }
#ifdef STATS
      cons_retries++;
#endif
#ifdef DEBUG_RING_BUFFER
      g_debug ("Consumer had to wait");
#endif
      if (error_has_occurred ()) {
        g_set_error (error, MD5COPY_ERROR, MD5COPY_ERROR_GENERIC, "Some error occurred while the consumer was waiting for a buffer");
        return NULL;
      }
    }
  }
  if (!got_signal) {
    g_set_error (error, MD5COPY_ERROR, MD5COPY_ERROR_CONSUMER_TIMEOUT,
                 "Consumer could not find any work even after %d retries!", MAX_CONSUMER_RETRIES);
    g_mutex_unlock (mutex);
    return NULL;
  }

  cons = t;

  g_cond_signal (cond_cons);
  g_mutex_unlock (mutex);

  return buffers[cons];
}

#ifdef STATS
gchar *
ring_buffer_get_stats ()
{
  return g_strdup_printf ("%d producer waits, \t %d consumer waits, \t %d consumer retries",  prod_waits, cons_waits, cons_retries);
}
#endif

void
ring_buffer_free ()
{
  g_free (buffers);

  g_mutex_free (mutex);
  g_cond_free (cond_prod);
  g_cond_free (cond_cons);
}
