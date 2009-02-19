/* $Id: ring-buffer-test.c 37 2008-07-20 17:17:02Z squisher $ */
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

#include <glib.h>
#include <stdio.h>

#include "global.h"
#include "ring-buffer.h"

void producer (workunit *init)
{
  int max = 128;
  int i;
  workunit *wu;

  wu = init;

  for (i = 0; i< max; i++ ) {
    wu->buf[1] = '\0';
    wu->buf[0] = 'x';
    g_usleep (10000 * g_random_int_range (1,11));
    wu->buf[0] = '0' + (i%10);
    wu = ring_buffer_put ();
  }
}

void thread_consumer ()
{
  workunit * wu;

  //g_usleep (500000);

  while (1) {
    //printf ("wait...");
    fflush (stdout);
    wu = ring_buffer_get ();
    //printf ("done...");
    if (wu->buf[0] < '0' || wu->buf[1] > '9')
      g_error ("unexpected value");
    printf ("%s ", wu->buf);
    fflush (stdout);
    g_usleep (10000 * g_random_int_range (1,11));
  }
}

int main ()
{
  workunit *buf;

  g_thread_init (NULL);

  buf = ring_buffer_init ();

  g_thread_create ((GThreadFunc) thread_consumer, NULL, FALSE, NULL);

  g_usleep (50000);

  producer (buf);

  return 0;
}
