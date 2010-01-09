/*
 *  Copyright (C) 2009 David Mohr <david@mcbf.net>
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
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib.h>

#include "ompa-list.h"

/*- private prototypes -*/



/*- private implementations -*/


/*- public functions -*/

GList *
ompa_list_filter (GList * list, OmpaFilterFunc pred)
{
  GList * newlist = list;
  GList * elem, * next;

  for (elem = list; elem != NULL; elem = next) {
    next = elem->next;

    if (!pred (elem->data)) {
      newlist = g_list_delete_link (newlist, elem);
    }
  }

  return newlist;
}
