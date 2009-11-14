/* $Id: progress-dialog.c 53 2009-02-16 09:39:17Z squisher $ */
/*
 *  Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
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
#  include <config.h>
#else
# error "no config.h"
#endif

#ifdef HAVE_STRING_H
#  include <string.h>
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <gtk/gtk.h>
#include <libxfcegui4/libxfcegui4.h>

#include "progress-dialog.h"

#define BORDER 10
#define BUF_SIZE 8192
#define PROGRESS_UPDATE_NUM 5
#define MIN_BLOCKS_PER_UPDATE 4
#define MAX_FILENAME_LEN 80

#define PROGRESS_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_PROGRESS_DIALOG, ProgressDialogPrivate))

/* struct */
typedef struct
{
  GtkWidget *progress_bar;
  GtkWidget *status_label;
  GtkWidget *file_label;
  GtkWidget *button_close;

  guint64 total_size;
  guint64 curr_size;
  gint i;

  gssize bytes_per_percent;

  GTimeVal tv_start;
  GTimeVal tv_end;

  ProgressDialogStatus status;
} ProgressDialogPrivate;


/* private prototypes */

static void progress_dialog_class_init (ProgressDialogClass * klass);
static void progress_dialog_init (ProgressDialog * sp);

static void progress_dialog_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void progress_dialog_set_property (GObject * object, guint prop_id, const GValue * value,
                                                 GParamSpec * pspec);

/*
static void cb_button_close_clicked (GtkWidget *button, ProgressDialog * dialog);
*/
static gboolean cb_dialog_delete (ProgressDialog * dialog, GdkEvent * event, ProgressDialogPrivate * priv);
static void progress_dialog_set_status (ProgressDialog * dialog, ProgressDialogStatus status);
static void progress_dialog_add_size (ProgressDialog * dialog, guint64 size);
static void progress_dialog_set_status_with_text (ProgressDialog * dialog, ProgressDialogStatus status, const gchar * text);
static void progress_dialog_set_filename (ProgressDialog * dialog, const gchar * fn);

/* globals */

enum
{
  PROP_0,
  PROP_STATUS,
  PROP_SIZE,
};

/*                                    */
/* enumeration type for dialog status */
/*                                    */
GType
progress_dialog_status_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    static const GEnumValue values[] = {
      {PROGRESS_DIALOG_STATUS_INIT, "PROGRESS_DIALOG_STATUS_INIT", "init"},
      {PROGRESS_DIALOG_STATUS_RUNNING, "PROGRESS_DIALOG_STATUS_RUNNING", "running"},
      {PROGRESS_DIALOG_STATUS_FAILED, "PROGRESS_DIALOG_STATUS_FAILED", "failed"},
      {PROGRESS_DIALOG_STATUS_CANCELLED, "PROGRESS_DIALOG_STATUS_CANCELLED", "cancelled"},
      {PROGRESS_DIALOG_STATUS_COMPLETED, "PROGRESS_DIALOG_STATUS_COMPLETED", "completed"},
      {0, NULL, NULL}
    };
    type = g_enum_register_static ("ProgressDialogStatus", values);
  }
  return type;
}

/*                       */
/* progress dialog class */
/*                       */
static GtkDialogClass *parent_class = NULL;

GType
progress_dialog_get_type (void)
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (ProgressDialogClass),
      NULL,
      NULL,
      (GClassInitFunc) progress_dialog_class_init,
      NULL,
      NULL,
      sizeof (ProgressDialog),
      0,
      (GInstanceInitFunc) progress_dialog_init,
      NULL
    };

    type = g_type_register_static (GTK_TYPE_DIALOG, "ProgressDialog", &our_info, 0);
  }

  return type;
}

static void
progress_dialog_class_init (ProgressDialogClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private (klass, sizeof (ProgressDialogPrivate));
  
  object_class->get_property = progress_dialog_get_property;
  object_class->set_property = progress_dialog_set_property;

  /* properties */
  g_object_class_install_property (object_class, PROP_STATUS,
                                   g_param_spec_enum ("status", "Status", "Status", TYPE_PROGRESS_DIALOG_STATUS,
                                                      PROGRESS_DIALOG_STATUS_INIT, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SIZE,
                                   g_param_spec_uint64 ("total-size", "Total Size", "Size of the whole operation",
                                                         0, -1, 0, G_PARAM_READWRITE)); 
  /*
  g_object_class_install_property (object_class, PROP_ANIMATE,
                                   g_param_spec_boolean ("animate", "Show an animation", "Show an animation",
                                                         FALSE, G_PARAM_READWRITE));
  */
}

static void
progress_dialog_init (ProgressDialog * obj)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (obj);
  GtkDialog *dialog = GTK_DIALOG (obj);
  GtkBox *box = GTK_BOX (dialog->vbox);
  GtkWidget *hbox;

  gtk_window_set_default_size (GTK_WINDOW (obj), 500, 75);

  /* label */
  priv->status_label = gtk_label_new ("Initializing ...");
  gtk_misc_set_alignment (GTK_MISC (priv->status_label), 0.1, 0.0);
  gtk_label_set_justify (GTK_LABEL (priv->status_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_selectable (GTK_LABEL (priv->status_label), TRUE);
  gtk_widget_show (priv->status_label);
  gtk_box_pack_start (box, priv->status_label, FALSE, TRUE, BORDER);

  /* progress bar */
  priv->progress_bar = gtk_progress_bar_new ();
  gtk_widget_show (priv->progress_bar);
  gtk_box_pack_start (box, priv->progress_bar, FALSE, FALSE, BORDER);
  gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (priv->progress_bar), 0.05);

  /* adjust the action_box, since we'll not just use it for buttons */
  gtk_container_remove (GTK_CONTAINER (box), dialog->action_area);
  dialog->action_area = hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_end (box, hbox, TRUE, TRUE, 0);
  gtk_box_reorder_child (box, hbox, 0);

  /* file label */
  
  priv->file_label = gtk_label_new ("...");
  gtk_misc_set_alignment (GTK_MISC (priv->file_label), 0.1, 0.0);
  gtk_label_set_justify (GTK_LABEL (priv->file_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_selectable (GTK_LABEL (priv->file_label), TRUE);
  gtk_widget_show (priv->file_label);
  //gtk_box_pack_start (GTK_BOX (GTK_DIALOG (obj)->action_area), priv->file_label, TRUE, TRUE, BORDER);
  gtk_box_pack_start (GTK_BOX (hbox), priv->file_label, TRUE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (priv->file_label), 0.0, 0.0);

  /* action buttons */
  priv->button_close = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (priv->button_close);
  //gtk_dialog_add_action_widget (GTK_DIALOG (obj), priv->button_close, GTK_RESPONSE_CANCEL);
  gtk_box_pack_end (GTK_BOX (hbox), priv->button_close, FALSE, FALSE, 0);
  GTK_WIDGET_SET_FLAGS (priv->button_close, GTK_CAN_DEFAULT);
  gtk_widget_grab_focus (priv->button_close);
  gtk_widget_grab_default (priv->button_close);

  g_signal_connect (G_OBJECT (priv->button_close), "clicked", G_CALLBACK (cb_dialog_delete), obj);

  gtk_widget_show (GTK_WIDGET (hbox));
  
  g_signal_connect (G_OBJECT (obj), "delete-event", G_CALLBACK (cb_dialog_delete), priv);
}

static void
progress_dialog_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (object);

  switch (prop_id) {
  case PROP_STATUS:
    g_value_set_enum (value, priv->status);
    break;
  case PROP_SIZE:
    g_value_set_uint64 (value, priv->total_size);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
progress_dialog_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  ProgressDialog *dialog = PROGRESS_DIALOG (object);
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);

  switch (prop_id) {
  case PROP_STATUS:
    progress_dialog_set_status (dialog, g_value_get_enum (value));
    break;
  case PROP_SIZE:
    priv->total_size =  g_value_get_uint64(value);

    priv->bytes_per_percent = priv->total_size / 100;
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
set_action_text (ProgressDialog * dialog, ProgressDialogStatus status, const gchar * text)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);
  gchar *temp = NULL;
  gchar *text_time = NULL;

  if (priv->status > PROGRESS_DIALOG_STATUS_RUNNING) {
    GTimeVal tv;
    guint64 per_sec;

    tv.tv_sec = priv->tv_end.tv_sec - priv->tv_start.tv_sec;
    tv.tv_usec = priv->tv_end.tv_usec;
    g_time_val_add (&tv, -1*priv->tv_start.tv_usec);

    if (tv.tv_sec != 0)
      per_sec = priv->total_size / tv.tv_sec;
    else { 
      per_sec = priv->total_size;
      if (tv.tv_usec >= G_USEC_PER_SEC / 2)
        tv.tv_sec = 1;
    }

    g_message ("Time elapsed: %ld.%lds", tv.tv_sec, tv.tv_usec);
    text_time = g_strdup_printf ("%s (%lluMB/s, %ldm%2lds)", text, per_sec / (1024*1024), tv.tv_sec / 60, tv.tv_sec % 60);
  } else {
    text_time = g_strdup (text);
  }

  if (status == PROGRESS_DIALOG_STATUS_FAILED)
    temp = g_strdup_printf ("<span size=\"larger\" foreground=\"red\">%s</span>", text_time);
  else
    temp = g_strdup_printf ("<b>%s</b>", text_time);

  gtk_label_set_markup (GTK_LABEL (priv->status_label), temp);

  g_free (temp);  
  g_free (text_time);  
}

static void
progress_dialog_add_size (ProgressDialog * dialog, guint64 size)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);
  gint cur_permil = 0;
  gint permil = 0;
  gchar *text = NULL;

  priv->curr_size += size;

  cur_permil = (gint) (gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (priv->progress_bar)) * 1000.0);
  permil = (gint) ((gdouble) priv->curr_size / (gdouble) priv->total_size * 1000.0);

  if (permil >= 1000) {
    permil = 1000;
    switch (priv->status) {
    case PROGRESS_DIALOG_STATUS_INIT:
      g_error ("Done while initializing");
      break;
    case PROGRESS_DIALOG_STATUS_CALCULATING_SIZE:
      g_error ("Done while calculating size");
      break;
    case PROGRESS_DIALOG_STATUS_COPYING:
    case PROGRESS_DIALOG_STATUS_RUNNING:
      text = g_strdup ("100%");
      break;
    case PROGRESS_DIALOG_STATUS_FAILED:
      text = g_strdup (_("Failed"));
      break;
    case PROGRESS_DIALOG_STATUS_CANCELLED:
      text = g_strdup (_("Cancelled"));
      break;
    case PROGRESS_DIALOG_STATUS_COMPLETED:
      text = g_strdup (_("Completed"));
      break;
    }
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar), permil / 1000.0);
  }
  else if (permil < 0) {
    permil = 0;
    text = g_strdup ("0%");
    g_warning ("Negative progress");
  }
  // FIXME: improve the fraction >= cur_fraction check, which will pretty much always succeed because it's a double
  else if (priv->status == PROGRESS_DIALOG_STATUS_COPYING && permil >= cur_permil) {
    text = g_strdup_printf ("%d%%  ", permil / 10);
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar), (gdouble) permil / 1000.0);
  }
  else if (permil < cur_permil) {
    return;
  }

  if (text != NULL) {
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), text);
    g_free (text);
  }
}

static void 
progress_dialog_set_filename (ProgressDialog * dialog, const gchar * fn)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);
  int len;

  len = strlen (fn);
  if (len > MAX_FILENAME_LEN) {
    gchar *truncated;

    truncated = g_strdup_printf ("...%s", fn + (len - MAX_FILENAME_LEN));
    gtk_label_set_text (GTK_LABEL (priv->file_label), truncated);
    g_free (truncated);
  } else 
    gtk_label_set_text (GTK_LABEL (priv->file_label), fn);
}

/* callbacks */
/*
static void
cb_button_close_clicked (GtkWidget *button, ProgressDialog *dialog)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));
}
*/

static gboolean
cb_dialog_delete (ProgressDialog * dialog, GdkEvent * event, ProgressDialogPrivate * priv)
{
  /*
  if (!GTK_WIDGET_SENSITIVE (priv->button_close)) {
    gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
    return TRUE;
  } else {
    gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
    return FALSE;
  }
  */
  gtk_main_quit ();
  return FALSE;
}

static void
progress_dialog_set_status_with_text (ProgressDialog * dialog, ProgressDialogStatus status, const gchar * text)
{
  progress_dialog_set_status (dialog, status);
  set_action_text (dialog, status, text);
}

static void
progress_dialog_set_status (ProgressDialog * dialog, ProgressDialogStatus status)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);

  priv->status = status;

  if (status == PROGRESS_DIALOG_STATUS_CALCULATING_SIZE) {
    set_action_text (dialog, status, "Calculating size....");
    g_get_current_time (&(priv->tv_start));
  } else {
    g_get_current_time (&(priv->tv_end));

    if (status == PROGRESS_DIALOG_STATUS_COPYING) {
      set_action_text (dialog, status, "Copying....");
    }
    gtk_button_set_label (GTK_BUTTON (priv->button_close), GTK_STOCK_CLOSE);
  }

  if (status == PROGRESS_DIALOG_STATUS_COMPLETED) {
    //g_debug ("size = %llu / %llu", priv->curr_size, priv->total_size);
    
    // FIXME why add size? To update the status? Seems clumsy
    progress_dialog_add_size (dialog, 0);

    progress_dialog_set_filename (dialog, "");
  }
}

/*        */
/* public */
/*        */

/* getters */
gdouble
progress_dialog_get_progress_bar_fraction (ProgressDialog * dialog)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);
  gdouble ret = 0;

  ret = gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (priv->progress_bar));

  return ret;
}

/* setters */
void
progress_dialog_thread_add_size (ProgressDialog * dialog, guint64 size)
{
  gdk_threads_enter ();
  progress_dialog_add_size (dialog, size);
  gdk_threads_leave ();
}

void
progress_dialog_thread_set_status (ProgressDialog * dialog, ProgressDialogStatus status)
{
  gdk_threads_enter ();
  progress_dialog_set_status (dialog, status);
  gdk_threads_leave ();
}

void
progress_dialog_thread_set_status_with_text (ProgressDialog * dialog, ProgressDialogStatus status, const gchar * text)
{
  gdk_threads_enter ();
  progress_dialog_set_status_with_text (dialog, status, text);
  gdk_threads_leave ();
}

void 
progress_dialog_thread_set_filename (ProgressDialog * dialog, const gchar * fn)
{
  gdk_threads_enter ();
  progress_dialog_set_filename (dialog, fn);
  gdk_threads_leave ();
}

guint64
progress_dialog_thread_get_current_size (ProgressDialog * dialog)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);
  guint64 ret;

  gdk_threads_enter ();
  ret = priv->curr_size;
  gdk_threads_leave ();

  return ret;
}

/* constructor */
ProgressDialog *
progress_dialog_new (void)
{
  ProgressDialog *obj;
  obj = PROGRESS_DIALOG (g_object_new (TYPE_PROGRESS_DIALOG, "modal", TRUE, NULL));
    
  return obj;
}
