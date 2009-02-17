/* $Id: thread-hash.c 53 2009-02-16 09:39:17Z squisher $ */
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

#include "global.h"
#include "thread-hash.h"
#include "ring-buffer.h"
#include "error.h"

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
thread_hash (MHASH master_hash)
{
  workunit *wu_cur;
  FILE *fp = NULL;
  GSList *fp_list = NULL;
  MHASH hash;
  unsigned char digest[16]; // FIXME: 16 is only good for md5

  hash = mhash_cp (master_hash);

  while (!error_has_occurred ()) {
    wu_cur = ring_buffer_get ();

    if (G_UNLIKELY (wu_cur == NULL))
      thread_show_error ("Threads out of sync!");

    if (wu_cur->close) {
      g_assert (fp != NULL);

      fclose (fp);

      /* pop the previous fp off the stack */
      fp = (FILE *) fp_list->data;
      fp_list = g_slist_delete_link (fp_list, fp_list);

      /* reset the wu value */
      wu_cur->close = FALSE;
    }

    if (wu_cur->open_md5) {
      fp_list = g_slist_prepend (fp_list, fp);

      fp = fopen (wu_cur->open_md5, "w");
      if (G_UNLIKELY (fp == NULL))
        thread_show_error ("Error opening %s for writing the hash!", wu_cur->open_md5);
      g_free (wu_cur->open_md5);

      /* reset the wu value */
      wu_cur->open_md5 = NULL;
    }

    if (G_LIKELY (wu_cur->n))
      mhash (hash, wu_cur->buf, wu_cur->n);

    if (wu_cur->write_hash) {
      g_assert (fp != NULL);
      
      mhash_deinit (hash, digest);
      print_digest (fp, wu_cur->write_hash, digest);
      g_free (wu_cur->write_hash);

      /* reset the wu value */
      wu_cur->write_hash = NULL;

      /* clone the hash, for then next operation */
      hash = mhash_cp (master_hash);
    }

    if (wu_cur->quit) {
      if (fp_list != NULL)
        g_warning ("Stack of file pointers is not empty!");
      break;
    }
  }

  /* deallocate the mhash resources, as we alway have hash as a valid mhash instance */
  mhash_deinit (hash, NULL);
}

