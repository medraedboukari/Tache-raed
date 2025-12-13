#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <gtk/gtk.h>
#include "coach.h"

// ADD
void on_btadd_clicked(GtkButton *button, gpointer user_data);

// MODIFY
void on_btmodify_clicked(GtkButton *button, gpointer user_data);

// DELETE
void on_btdelete_clicked(GtkButton *button, gpointer user_data);

// SEARCH
void on_btsearch_clicked(GtkButton *button, gpointer user_data);

// RADIO BUTTONS
void on_radiobuttonman_toggled(GtkToggleButton *togglebutton, gpointer user_data);
void on_radiobuttonwomen_toggled(GtkToggleButton *togglebutton, gpointer user_data);

// TREEVIEW EVENTS
void on_treeviewmanage_row_activated(GtkTreeView *treeview,
                                     GtkTreePath *path,
                                     GtkTreeViewColumn *column,
                                     gpointer user_data);

// REFRESH BUTTON
void on_refresh_clicked(GtkButton *button, gpointer user_data);


#endif


void
on_treeview13_row_activated            (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data);

void
on_button39_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_button40_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_checkbutton_center_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_checkbutton_name_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);
