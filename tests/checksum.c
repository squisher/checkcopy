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

#include <libxfce4util/libxfce4util.h>

#include <gtk/gtk.h>
#include <glib.h>

#include <stdlib.h>

#include "checkcopy-input-stream.h"
#include "checkcopy-file-info.h"

static CheckcopyFileInfo test_files[] = {
  { "data/checkcopy", "718fa88fd9c3a3e8b8318b282b4d3817a5e3c140",
    CHECKCOPY_SHA1, 0, FALSE, FALSE },
  { "data/COPYING", "751419260aa954499f7abaabaa882bbe",
    CHECKCOPY_MD5, 0, FALSE, FALSE },
  { "data/COPYING", "ab15fd526bd8dd18a9e77ebc139656bf4d33e97fc7238cd11bf60e2b9b8666c6",
    CHECKCOPY_SHA256, 0, FALSE, FALSE },
  { "data/COPYING", "1765105c",
    CHECKCOPY_CRC32, 0, FALSE, FALSE },
  { NULL }
};

static void
input_stream_checksum(void)
{
  GInputStream *in;
  GFile *file;
  CheckcopyInputStream *cin;
  char buffer[8192];
  gsize read;
  GError *error = NULL;
  const char * checksum;
  CheckcopyFileInfo * info;

  g_assert (test_files != NULL);

  for (info = test_files; info->relname != NULL; info++) {
    g_print ("Testing %s...\n", info->relname);

    file = g_file_new_for_path (info->relname);
    g_assert (file != NULL);

    in = G_INPUT_STREAM (g_file_read (file, NULL, &error));
    g_assert_no_error (error);
    g_assert (in != NULL);

    cin = checkcopy_input_stream_new (in, CHECKCOPY_ALL_CHECKSUMS);
    g_assert (cin != NULL);

    for (read = -1; read != 0; )
    {
      gboolean r;

      r = g_input_stream_read_all (G_INPUT_STREAM (cin),
                                  buffer, 8192,
                                  &read, NULL, &error);
      g_assert (r);
      g_assert_no_error (error);
    }
    g_input_stream_close (G_INPUT_STREAM (cin), NULL, &error);
    g_assert_no_error (error);

    checksum = checkcopy_input_stream_get_checksum (cin, info->checksum_type);
    g_assert_cmpstr (checksum, ==, info->checksum);
  }
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_type_init ();

  g_test_add_func ("/input-stream/checksum", input_stream_checksum);

  return g_test_run ();
}
