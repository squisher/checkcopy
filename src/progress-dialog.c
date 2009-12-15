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
#include <libxfce4util/libxfce4util.h>

#include "progress-dialog.h"
#include "checkcopy-cancel.h"
#include "checkcopy-file-list.h"
#include "checkcopy-details-window.h"

#define BORDER 10
#define MAX_FILENAME_LEN 80

#define PROGRESS_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_PROGRESS_DIALOG, ProgressDialogPrivate))

/* struct */
typedef struct
{
  GtkWidget *progress_bar;
  GtkWidget *status_label;
  GtkWidget *file_label;
  GtkWidget *button_close;
  GtkWidget *button_details;
  GtkWidget *stats_entry[CHECKCOPY_FILE_LIST_COUNT_LAST];

  CheckcopyDetailsWindow *details;
  CheckcopyFileList * list;

  guint64 total_size;
  guint64 curr_size;
#ifdef DEBUG
  guint num_files;
#endif
  gint i;

  gssize bytes_per_percent;

  GTimeVal tv_start;
  GTimeVal tv_end;

  guint idle_tag;

  ProgressDialogStatus status;
  gboolean verify_only;
} ProgressDialogPrivate;


/* private prototypes */

static void progress_dialog_class_init (ProgressDialogClass * klass);
static void progress_dialog_init (ProgressDialog * obj);
static void progress_dialog_finalize (GObject * obj);

static void progress_dialog_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void progress_dialog_set_property (GObject * object, guint prop_id, const GValue * value,
                                                 GParamSpec * pspec);

static void cb_close_clicked (GtkButton * button, ProgressDialog * dialog);
static void cb_details_clicked (GtkButton * button, ProgressDialog * dialog);
static void cb_cancel (GCancellable *cancel, ProgressDialog * dialog);
static gboolean cb_delete (ProgressDialog * dialog, GdkEvent * event, gpointer data);
static gboolean cb_update_progress (gpointer data);
static void set_update (ProgressDialog * dialog, gboolean enabled);
static void update_stat_labels (ProgressDialog * dialog);

static GtkWidget * stats_label_new (const gchar * tooltip, GtkWidget *label);
static void update_progress (ProgressDialog * dialog);
static void progress_dialog_add_size (ProgressDialog * dialog, guint64 size);
static gboolean progress_dialog_set_status (ProgressDialog * dialog, ProgressDialogStatus status);
static gboolean progress_dialog_set_status_with_text (ProgressDialog * dialog, ProgressDialogStatus status, const gchar * text);
static void progress_dialog_set_filename (ProgressDialog * dialog, const gchar * fn);

/* globals */

enum
{
  PROP_0,
  PROP_STATUS,
  PROP_SIZE,
  PROP_VERIFY_ONLY,
  PROP_NUM_FILES,
};

gchar *stats_text[] = {
  N_("Verified:"),
  N_("Created:"),
  N_("Failed:"),
  N_("Not found:"),
};

gchar *stats_tooltip[] = {
  N_("Number of file(s) which were copied and verified successfully"),
  N_("Number of file(s) which were copied successfully but not verified by a checksum"),
  N_("Number of file(s) which failed copying and/or verification"),
  N_("Number of file(s) which were mentioned in a checksum file, but were not found"),
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
      {PROGRESS_DIALOG_STATUS_INIT, "PROGRESS_DIALOG_STATUS_INIT", N_("init")},
      {PROGRESS_DIALOG_STATUS_CALCULATING_SIZE, "PROGRESS_DIALOG_STATUS_CALCULATING_SIZE", N_("calculating-size")},
      {PROGRESS_DIALOG_STATUS_RUNNING, "PROGRESS_DIALOG_STATUS_RUNNING", N_("running")},
      {PROGRESS_DIALOG_STATUS_COPYING, "PROGRESS_DIALOG_STATUS_COPYING", N_("copying")},
      {PROGRESS_DIALOG_STATUS_FAILED, "PROGRESS_DIALOG_STATUS_FAILED", N_("failed")},
      {PROGRESS_DIALOG_STATUS_CANCELLED, "PROGRESS_DIALOG_STATUS_CANCELLED", N_("cancelled")},
      {PROGRESS_DIALOG_STATUS_COMPLETED, "PROGRESS_DIALOG_STATUS_COMPLETED", N_("completed")},
      {0, NULL, NULL}
    };
    type = g_enum_register_static ("ProgressDialogStatus", values);
  }
  return type;
}

/*                       */
/* progress dialog class */
/*                       */
static GtkWindowClass *parent_class = NULL;

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

    type = g_type_register_static (GTK_TYPE_WINDOW, "ProgressDialog", &our_info, 0);
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
  object_class->finalize = progress_dialog_finalize;

  /* properties */
  g_object_class_install_property (object_class, PROP_STATUS,
                                   g_param_spec_enum ("status", _("Status"), _("Status"), TYPE_PROGRESS_DIALOG_STATUS,
                                                      PROGRESS_DIALOG_STATUS_INIT, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SIZE,
                                   g_param_spec_uint64 ("total-size", _("Total Size"), _("Size of the whole operation"),
                                                         0, -1, 0, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_VERIFY_ONLY,
                                   g_param_spec_boolean ("verify-only", _("Verify only"), _("Verification only mode"),
                                                         FALSE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_NUM_FILES,
                                   g_param_spec_uint ("num-files", _("Number of files"), _("Number of files"),
                                   0, G_MAXUINT, 0, G_PARAM_WRITABLE));

}

static void
progress_dialog_init (ProgressDialog * obj)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (obj);
  GtkWidget *hbox, *vbox, *statbox;
  GCancellable *cancel;
  GtkWidget *label;
  GtkWidget *align;
  GtkWidget *sep;
  GtkWidget *entry;

  gint i;

  priv->list = checkcopy_file_list_get_instance ();

  gtk_window_set_default_size (GTK_WINDOW (obj), 500, 75);
  gtk_window_set_icon_name (GTK_WINDOW (obj), GTK_STOCK_SAVE_AS);

  vbox = gtk_vbox_new (FALSE, 0);
  //gtk_box_pack_end (GTK_BOX (obj), hbox, TRUE, TRUE, 5);
  gtk_container_add (GTK_CONTAINER (obj), vbox);
  //gtk_box_reorder_child (box, hbox, 0);

  /* label */
  priv->status_label = gtk_label_new (_("Initializing ..."));
  gtk_misc_set_alignment (GTK_MISC (priv->status_label), 0.1, 0.0);
  gtk_label_set_justify (GTK_LABEL (priv->status_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_selectable (GTK_LABEL (priv->status_label), TRUE);
  gtk_widget_show (priv->status_label);
  gtk_box_pack_start (GTK_BOX (vbox), priv->status_label, FALSE, TRUE, BORDER);

  /* progress bar */
  priv->progress_bar = gtk_progress_bar_new ();
  gtk_widget_show (priv->progress_bar);
  gtk_box_pack_start (GTK_BOX (vbox), priv->progress_bar, FALSE, FALSE, BORDER);
  gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (priv->progress_bar), 0.05);

  /* file label */

  priv->file_label = gtk_label_new ("...");
  gtk_misc_set_alignment (GTK_MISC (priv->file_label), 0.1, 0.0);
  gtk_label_set_justify (GTK_LABEL (priv->file_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_selectable (GTK_LABEL (priv->file_label), TRUE);
  //gtk_box_pack_start (GTK_BOX (GTK_DIALOG (obj)->action_area), priv->file_label, TRUE, TRUE, BORDER);
  gtk_box_pack_start (GTK_BOX (vbox), priv->file_label, TRUE, TRUE, 0);
  gtk_widget_show (priv->file_label);
  gtk_misc_set_alignment (GTK_MISC (priv->file_label), 0.0, 0.0);

  /* lower area */

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 2);
  gtk_widget_show (sep);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* details button */
  priv->button_details = gtk_button_new_with_label (_("Details"));
  gtk_widget_show (priv->button_details);
  gtk_box_pack_start (GTK_BOX (hbox), priv->button_details, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (priv->button_details), "clicked", G_CALLBACK (cb_details_clicked), obj);

  /* stats */
  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), align, TRUE, TRUE, 0);
  gtk_widget_show (align);

  statbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (align), statbox);
  gtk_widget_show (statbox);

  /* -- */
  for (i = 0; i < CHECKCOPY_FILE_LIST_COUNT_LAST; i++ ) {
    label = gtk_label_new (_(stats_text[i]));
    gtk_misc_set_padding (GTK_MISC (label), 4, 0);
    gtk_box_pack_start (GTK_BOX (statbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    priv->stats_entry[i] = entry = stats_label_new (_(stats_tooltip[i]),
                                                    label);
    gtk_box_pack_start (GTK_BOX (statbox), entry, FALSE, FALSE, 0);
    gtk_widget_show (entry);
  }

  /* close button */
  priv->button_close = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (priv->button_close);
  gtk_box_pack_end (GTK_BOX (hbox), priv->button_close, FALSE, FALSE, 0);
#if 0
  GTK_WIDGET_SET_FLAGS (priv->button_close, GTK_CAN_DEFAULT);
  gtk_widget_grab_focus (priv->button_close);
  gtk_widget_grab_default (priv->button_close);
#endif

  g_signal_connect (G_OBJECT (priv->button_close), "clicked", G_CALLBACK (cb_close_clicked), obj);

  gtk_widget_show (GTK_WIDGET (vbox));


  g_signal_connect (G_OBJECT (obj), "delete-event", G_CALLBACK (cb_delete), NULL);

  priv->details = checkcopy_details_window_new ();

  cancel = checkcopy_get_cancellable ();

  if (g_cancellable_is_cancelled (cancel)) {
    cb_cancel (cancel, obj);
  } else {
#if 0
    g_cancellable_connect (cancel, G_CALLBACK (cb_cancel), obj, NULL);
#endif
  }
}

static void
progress_dialog_finalize (GObject * obj)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (obj);

  g_object_unref (priv->list);
  g_object_unref (priv->details);

  G_OBJECT_CLASS (parent_class)->finalize (obj);
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
  case PROP_VERIFY_ONLY:
    g_value_set_boolean (value, priv->verify_only);
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
  case PROP_VERIFY_ONLY:
    priv->verify_only = g_value_get_boolean (value);
    break;
  case PROP_NUM_FILES:
    {
#if 0
      gchar *s;
      s = g_strdup_printf ("%4d", g_value_get_uint (value));
      gtk_entry_set_text (GTK_ENTRY (priv->entry_total), s);
      g_free (s);
#endif
    }
#ifdef DEBUG
    priv->num_files = g_value_get_uint (value);
#endif
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

  if (priv->status >= PROGRESS_DIALOG_STATUS_FAILED) {
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
    text_time = g_strdup_printf (_("%s (%lluMB/s, %ldm%2lds)"), text, per_sec / (1024*1024), tv.tv_sec / 60, tv.tv_sec % 60);
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

  priv->curr_size += size;
}

static GtkWidget *
stats_label_new (const gchar * tooltip, GtkWidget *label)
{
  GtkWidget * entry;

  entry = gtk_entry_new ();

  g_object_set (G_OBJECT (entry), "editable", FALSE,
                                  "width-chars", 5,
                                  "has-frame", TRUE,
                                  "shadow-type", GTK_SHADOW_NONE,
                                  NULL);

  gtk_widget_set_tooltip_text (label, tooltip);
  gtk_widget_set_tooltip_text (entry, tooltip);


  return entry;
}

static void
update_progress (ProgressDialog * dialog)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);
  gint cur_permil = 0;
  gint permil = 0;
  gchar *text = NULL;

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
  else if (priv->status == PROGRESS_DIALOG_STATUS_COPYING && permil >= cur_permil) {
    GtkProgressBar *pbar = GTK_PROGRESS_BAR (priv->progress_bar);
    text = g_strdup_printf ("%d%%  ", permil / 10);
    gtk_progress_bar_set_fraction (pbar, (gdouble) permil / 1000.0);

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

static void
update_stat_labels (ProgressDialog * dialog)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);

  CheckcopyFileListStats * stats;
  gchar *s;
  const gchar fmt[] = "%4d";
  GdkColor color;
  const gchar * color_str;
  gint i;

  stats = checkcopy_file_list_get_stats (priv->list);

  for (i = 0; i < CHECKCOPY_FILE_LIST_COUNT_LAST; i++) {
    s = g_strdup_printf (fmt, stats->count[i]);
    gtk_entry_set_text (GTK_ENTRY (priv->stats_entry[i]), s);
    g_free (s);

    if (stats->count[i] > 0) {
      if ((color_str = checkcopy_file_info_status_color (checkcopy_file_list_status_to_info(i))) != NULL) {
        gdk_color_parse (color_str, &color);
        gtk_widget_modify_base (priv->stats_entry[i], GTK_STATE_NORMAL, &color);
      }
      gtk_widget_set_sensitive (priv->stats_entry[i], TRUE);
    } else {
      /* not_found == 0 */

      gtk_widget_set_sensitive (priv->stats_entry[i], FALSE);
    }

  } /* end for */

  g_free (stats);
}

/* callbacks */

static void
cb_close_clicked (GtkButton * button, ProgressDialog * dialog)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);
  GCancellable *cancel;

  cancel = checkcopy_get_cancellable ();

  if (g_strcmp0 (gtk_button_get_label (GTK_BUTTON (priv->button_close)), GTK_STOCK_CLOSE) == 0 || g_cancellable_is_cancelled (cancel)) {
    gtk_main_quit ();
  } else {
    g_cancellable_cancel (cancel);
  }
}

static void
cb_details_clicked (GtkButton * button, ProgressDialog * dialog)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);

  g_object_set (G_OBJECT (priv->details), "file-info-list",
                checkcopy_file_list_get_sorted_list (priv->list), NULL);

  gtk_widget_show (GTK_WIDGET (priv->details));
}

static gboolean
cb_delete (ProgressDialog * dialog, GdkEvent * event, gpointer data)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);

  cb_close_clicked (GTK_BUTTON (priv->button_close), dialog);

  return FALSE;
}

static void
cb_cancel (GCancellable *cancel, ProgressDialog * dialog)
{
  //ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);

  DBG ("Cancelled....");


  progress_dialog_set_status_with_text (dialog, PROGRESS_DIALOG_STATUS_CANCELLED, _("Cancelled"));
}

static gboolean
cb_update_progress (gpointer data)
{
  ProgressDialog * dialog = PROGRESS_DIALOG (data);
  //ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);

  /* idle functions are executed outside of the gdk lock */
  gdk_threads_enter ();
  //DBG ("Updating progress");
  update_progress (dialog);
  update_stat_labels (dialog);
  gdk_threads_leave ();

  return TRUE;
}

static void
set_update (ProgressDialog * dialog, gboolean enabled)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);

  if (enabled) {
    priv->idle_tag = g_timeout_add_full (G_PRIORITY_LOW, 250, cb_update_progress, dialog, NULL);
  } else {
    /* disable */

    gboolean r;

    g_assert (priv->idle_tag != 0);

    r = g_source_remove (priv->idle_tag);

    if (!r) {
      g_warning ("Unable to remove update idle function");
    }

    priv->idle_tag = 0;

    /* run the update one last time, to ensure all info is up-to-date */
    update_progress (dialog);
    update_stat_labels (dialog);
  }
}

static gboolean
progress_dialog_set_status_with_text (ProgressDialog * dialog, ProgressDialogStatus status, const gchar * text)
{
  gboolean ret;

  ret = progress_dialog_set_status (dialog, status);
  if (ret) {
    set_action_text (dialog, status, text);
    return TRUE;
  } else {
    return FALSE;
  }
}

static gboolean
progress_dialog_set_status (ProgressDialog * dialog, ProgressDialogStatus status)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);

  if (priv->status == PROGRESS_DIALOG_STATUS_CANCELLED) {
    /* cancelled is a trapped state, normally we don't exit */
    return FALSE;
  }

  priv->status = status;

  if (status == PROGRESS_DIALOG_STATUS_CALCULATING_SIZE) {
    set_action_text (dialog, status, _("Calculating size...."));
    g_get_current_time (&(priv->tv_start));
  } else {
    g_get_current_time (&(priv->tv_end));

    if (status == PROGRESS_DIALOG_STATUS_COPYING) {
      set_action_text (dialog, status, _("Copying...."));
      set_update (dialog, TRUE);
    }
  }

  if (status == PROGRESS_DIALOG_STATUS_COMPLETED) {
    //g_debug ("size = %llu / %llu", priv->curr_size, priv->total_size);

    progress_dialog_set_filename (dialog, "");
    set_action_text (dialog, status, _("Done."));
    gtk_button_set_label (GTK_BUTTON (priv->button_close), GTK_STOCK_CLOSE);
    set_update (dialog, FALSE);
  } else if (status == PROGRESS_DIALOG_STATUS_CANCELLED) {
    set_action_text (dialog, status, _("Cancelled"));
    gtk_button_set_label (GTK_BUTTON (priv->button_close), GTK_STOCK_CLOSE);
  }

  return TRUE;
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

gboolean
progress_dialog_thread_set_status (ProgressDialog * dialog, ProgressDialogStatus status)
{
  gboolean ret;

  gdk_threads_enter ();
  ret = progress_dialog_set_status (dialog, status);
  gdk_threads_leave ();

  return ret;
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

void
progress_dialog_thread_set_done (ProgressDialog * dialog)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);

  /* reset the status to force moving it to complete */
  priv->status = PROGRESS_DIALOG_STATUS_COPYING;

  gdk_threads_enter ();
  progress_dialog_set_status (dialog, PROGRESS_DIALOG_STATUS_COMPLETED);
  gdk_threads_leave ();
}

#ifdef DEBUG
gboolean
progress_dialog_thread_check_stats (ProgressDialog * dialog)
{
  ProgressDialogPrivate *priv = PROGRESS_DIALOG_GET_PRIVATE (dialog);

  if (priv->status == PROGRESS_DIALOG_STATUS_COMPLETED) {
    guint stats_total;
    const CheckcopyFileListStats * stats;

    stats = checkcopy_file_list_get_stats (priv->list);

    stats_total = stats->copied + stats->failed + stats->verified;

    if (stats_total != priv->num_files) {
      g_warning ("Total number of files is %u, but stats were recorded for only %u",
                 priv->num_files, stats_total);
      return FALSE;
    } else {
      DBG ("Number of files and sum of stats matched up");
      return TRUE;
    }
  }

  /* Any other status cannot be verified */

  return TRUE;
}
#endif

/* constructor */
ProgressDialog *
progress_dialog_new (gboolean verify_only)
{
  ProgressDialog *obj;
  obj = PROGRESS_DIALOG (g_object_new (TYPE_PROGRESS_DIALOG, "verify-only", verify_only, NULL));

  return obj;
}
