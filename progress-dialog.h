/* $Id: progress-dialog.h 37 2008-07-20 17:17:02Z squisher $ */
/*
 *  Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
 *                2008      David Mohr <david@mcbf.net>
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

#ifndef __PROGRESS_DIALOG_H__
#define __PROGRESS_DIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS
/* */
#define TYPE_PROGRESS_DIALOG_STATUS (progress_dialog_status_get_type ())
typedef enum
{
  PROGRESS_DIALOG_STATUS_INIT,
  PROGRESS_DIALOG_STATUS_RUNNING,
  PROGRESS_DIALOG_STATUS_FAILED,
  PROGRESS_DIALOG_STATUS_CANCELLED,
  PROGRESS_DIALOG_STATUS_COMPLETED
} ProgressDialogStatus;

GType progress_dialog_status_get_type (void);


/* */
#define TYPE_PROGRESS_DIALOG         (progress_dialog_get_type ())
#define PROGRESS_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_PROGRESS_DIALOG, ProgressDialog))
#define PROGRESS_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_PROGRESS_DIALOG, ProgressDialogClass))
#define IS_PROGRESS_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_PROGRESS_DIALOG))
#define IS_PROGRESS_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_PROGRESS_DIALOG))
#define PROGRESS_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_PROGRESS_DIALOG, ProgressDialogClass))

typedef struct
{
  GtkDialog parent;
} ProgressDialog;

typedef struct
{
  GtkDialogClass parent_class;
} ProgressDialogClass;


GtkType progress_dialog_get_type ();

void progress_dialog_show_buffers (ProgressDialog * dialog, gboolean show);
void progress_dialog_pulse_progress_bar (ProgressDialog * dialog);

gdouble progress_dialog_get_progress_bar_fraction (ProgressDialog * dialog);

void progress_dialog_add_size (ProgressDialog * dialog, guint64 size);
void progress_dialog_set_status (ProgressDialog * dialog, ProgressDialogStatus status);
void progress_dialog_set_status_with_text (ProgressDialog * dialog, ProgressDialogStatus status, const gchar * text);
void progress_dialog_set_filename (ProgressDialog * dialog, const gchar * fn);

GtkWidget *progress_dialog_new ();

G_END_DECLS

#endif /* PROGRESS_DIALOG_H */
