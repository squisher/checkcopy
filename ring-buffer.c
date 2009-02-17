#include <glib.h>
#include <stdlib.h>

#include "global.h"
#include "ring-buffer.h"
#include "settings.h"


#if DEBUG > 1
# define DEBUG_RING_BUFFER
#endif


RING_BUFFER_TYPE **buffers; //[BUF_SLOTS][BUF_SIZE];
GMutex *mutex = NULL;
GCond *cond_prod = NULL;
GCond *cond_cons = NULL;
gboolean done = FALSE;
int prod, cons;

#ifdef STATS
int prod_waits, cons_waits;
#endif


RING_BUFFER_TYPE *
ring_buffer_init ()
{
  int i;
  buffers = malloc ( BUF_SLOTS * sizeof (void *));
  for (i=0; i<BUF_SLOTS; i++) {
    buffers[i] = g_new0 (RING_BUFFER_TYPE, 1);
  }
  mutex = g_mutex_new ();
  cond_prod = g_cond_new ();
  cond_cons = g_cond_new ();

  prod = 0;
  cons = BUF_SLOTS - 1;
  //cons = 0;

#ifdef STATS
  prod_waits = cons_waits = 0;
#endif
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
ring_buffer_get ()
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
#if DEBUG_RING_BUFFER
      if (cons_waits < 10)
        g_debug ("consumer wait %d", cons_waits);
#endif
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
#ifdef DEBUG_RING_BUFFER
      g_debug ("Consumer got stuck");
#endif
    }
  }
  if (!got_signal) {
    g_warning("Consumer could not find any work even after %d retries!", MAX_CONSUMER_RETRIES);
    g_mutex_unlock (mutex);
    return NULL;
  }

  cons = t;

  g_cond_signal (cond_cons);
  g_mutex_unlock (mutex);

  return buffers[cons];
}

#ifdef STATS
int
ring_buffer_prod_waits ()
{
  return prod_waits;
}

int
ring_buffer_cons_waits ()
{
  return cons_waits;
}
#endif
