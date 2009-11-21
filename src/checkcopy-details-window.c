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

/*- private prototypes -*/

static gboolean cb_delete (CheckcopyDetailsWindow * details, GdkEvent * event, gpointer data);
static void cb_close_clicked (GtkButton * button, CheckcopyDetailsWindow *details);

/*- globals -*/

enum {
  PROP_0,
};


/*****************/
/*- class setup -*/
/*****************/

G_DEFINE_TYPE (CheckcopyDetailsWindow, checkcopy_details_window, GTK_TYPE_WINDOW)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHECKCOPY_TYPE_DETAILS_WINDOW, CheckcopyDetailsWindowPrivate))

typedef struct _CheckcopyDetailsWindowPrivate CheckcopyDetailsWindowPrivate;

struct _CheckcopyDetailsWindowPrivate {
  int dummy;
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
  //CheckcopyDetailsWindowPrivate *priv = GET_PRIVATE (CHECKCOPY_DETAILS_WINDOW (object));

  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
checkcopy_details_window_finalize (GObject *object)
{
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
}

static void
checkcopy_details_window_init (CheckcopyDetailsWindow *self)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *top_frame;
  GtkWidget *sep;
  GtkWidget *button_close;

  gtk_window_set_title (GTK_WINDOW (self), "Details");
  gtk_window_set_default_size (GTK_WINDOW (self), 400, 300);

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


  g_signal_connect (G_OBJECT (self), "delete-event", G_CALLBACK (cb_delete), NULL);
}


/***************/
/*- internals -*/
/***************/

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
