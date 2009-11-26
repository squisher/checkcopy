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

#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>

#include "checkcopy-details-window.h"
#include "checkcopy-file-info.h"

/*- private prototypes -*/

static void display_list (CheckcopyDetailsWindow * details, GList *file_infos);
static void status_to_string (GtkTreeViewColumn * col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer data);

static gboolean cb_delete (CheckcopyDetailsWindow * details, GdkEvent * event, gpointer data);
static void cb_close_clicked (GtkButton * button, CheckcopyDetailsWindow *details);

/*- globals -*/

enum {
  PROP_0,
  PROP_FILE_INFO_LIST,
};

enum {
  COLUMN_RELNAME,
  COLUMN_HASH,
  COLUMN_STATUS,
  N_COLUMNS
};

/*****************/
/*- class setup -*/
/*****************/

G_DEFINE_TYPE (CheckcopyDetailsWindow, checkcopy_details_window, GTK_TYPE_WINDOW)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHECKCOPY_TYPE_DETAILS_WINDOW, CheckcopyDetailsWindowPrivate))

typedef struct _CheckcopyDetailsWindowPrivate CheckcopyDetailsWindowPrivate;

struct _CheckcopyDetailsWindowPrivate {
  GtkListStore * store;
  GtkWidget * view;
};

static void
checkcopy_details_window_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  //CheckcopyDetailsWindowPrivate *priv = GET_PRIVATE (CHECKCOPY_DETAILS_WINDOW (object));

  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
checkcopy_details_window_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  CheckcopyDetailsWindow * details = CHECKCOPY_DETAILS_WINDOW (object);
  //CheckcopyDetailsWindowPrivate *priv = GET_PRIVATE (details);

  switch (property_id) {
    case PROP_FILE_INFO_LIST:
      display_list (details, g_value_get_pointer (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
checkcopy_details_window_finalize (GObject *object)
{
  CheckcopyDetailsWindowPrivate *priv = GET_PRIVATE (CHECKCOPY_DETAILS_WINDOW (object));

  g_object_unref (priv->store);

  G_OBJECT_CLASS (checkcopy_details_window_parent_class)->finalize (object);
}

static void
checkcopy_details_window_class_init (CheckcopyDetailsWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CheckcopyDetailsWindowPrivate));

  object_class->get_property = checkcopy_details_window_get_property;
  object_class->set_property = checkcopy_details_window_set_property;
  object_class->finalize = checkcopy_details_window_finalize;

  g_object_class_install_property (object_class, PROP_FILE_INFO_LIST,
           g_param_spec_pointer ("file-info-list", "List of file infos", "List of file infos", G_PARAM_WRITABLE));
}

static void
checkcopy_details_window_init (CheckcopyDetailsWindow *self)
{
  CheckcopyDetailsWindowPrivate *priv = GET_PRIVATE (self);

  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *top_frame;
  GtkWidget *sep;
  GtkWidget *button_close;
  GtkWidget *scrolled;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeView *view;

  gtk_window_set_title (GTK_WINDOW (self), "Details");
  gtk_window_set_default_size (GTK_WINDOW (self), 600, 500);

  /* basic layout */

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (self), vbox);
  gtk_widget_show (vbox);

  top_frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), top_frame, TRUE, TRUE, 0);
  gtk_widget_show (top_frame);

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 0);
  gtk_widget_show (sep);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button_close = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  gtk_box_pack_end (GTK_BOX (hbox), button_close, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button_close), "clicked", G_CALLBACK (cb_close_clicked), self);
  gtk_widget_grab_focus (button_close);
  gtk_widget_show (button_close);

  /* list store */
  priv->store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

  /* tree view */
  priv->view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (priv->store));
  view = GTK_TREE_VIEW (priv->view);
  gtk_tree_view_set_headers_clickable (view, TRUE);
  gtk_tree_view_set_search_column (view, COLUMN_RELNAME);
  gtk_tree_view_set_enable_search (view, TRUE);
  renderer = gtk_cell_renderer_text_new ();

  column = gtk_tree_view_column_new_with_attributes ("Filename", renderer, "text", COLUMN_RELNAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (priv->view), column);

  column = gtk_tree_view_column_new_with_attributes ("Hash", renderer, "text", COLUMN_HASH, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (priv->view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Status", renderer, "text", COLUMN_STATUS, NULL);
  gtk_tree_view_column_set_cell_data_func (column, renderer, status_to_string, self, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (priv->view), column);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (top_frame), scrolled);
  gtk_widget_show (scrolled);

  gtk_container_add (GTK_CONTAINER (scrolled), priv->view);
  gtk_widget_show (priv->view);


  g_signal_connect (G_OBJECT (self), "delete-event", G_CALLBACK (cb_delete), NULL);
}


/***************/
/*- internals -*/
/***************/

static void
display_list (CheckcopyDetailsWindow * details, GList *file_infos)
{
  CheckcopyDetailsWindowPrivate * priv = GET_PRIVATE (details);

  GList * file_info;

  gtk_list_store_clear (priv->store);

  for (file_info = file_infos; file_info != NULL; file_info = g_list_next (file_info)) {
    GtkTreeIter iter;
    //GtkTreePath *path;
    CheckcopyFileInfo *info = (CheckcopyFileInfo *) file_info->data;

    gtk_list_store_append (priv->store, &iter);
    gtk_list_store_set (priv->store, &iter,
                        COLUMN_RELNAME, info->relname,
                        COLUMN_HASH, info->checksum,
                        COLUMN_STATUS, info->status,
                        -1);
  }
}

static void
status_to_string (GtkTreeViewColumn * col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  //CheckcopyDetailsWindow * details = CHECKCOPY_DETAILS_WINDOW (data);

  CheckcopyFileStatus status;
  gchar *color = NULL;

  gtk_tree_model_get (model, iter, COLUMN_STATUS, &status, -1);

  g_object_set (G_OBJECT (renderer), "text", checkcopy_file_status_to_string (status), NULL);
  
  switch (status) {
    case CHECKCOPY_STATUS_VERIFICATION_FAILED:
      color = "#ee2211";
      break;
    case CHECKCOPY_STATUS_VERIFIED:
      color = "#11ee22";
      break;
    default:
      break;
  }

  if (color) {
    g_object_set (G_OBJECT (renderer), "background", color, "background-set", TRUE, NULL);
  } else {
    g_object_set (G_OBJECT (renderer), "background-set", FALSE, NULL);
  }
}

static void
cb_close_clicked (GtkButton * button, CheckcopyDetailsWindow *details)
{
  DBG ("Close clicked");
  gtk_widget_hide (GTK_WIDGET (details));
}

static gboolean
cb_delete (CheckcopyDetailsWindow * details, GdkEvent * event, gpointer data)
{
  DBG ("Delete event");
  gtk_widget_hide (GTK_WIDGET (details));

  return FALSE;
}


/*******************/
/*- public methods-*/
/*******************/

CheckcopyDetailsWindow*
checkcopy_details_window_new (void)
{
  return g_object_new (CHECKCOPY_TYPE_DETAILS_WINDOW, NULL);
}
