/*
 * Copyright (C) 2009 Nils Björklund
 *
 * This file is part of Drumscope.
 *
 * Drumscope is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Drumscope is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Drumscope.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "drumscope-actor.h"
#include "drum-io.h"

#include <gtk/gtk.h>
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>

#include <stdlib.h>

#define NR_OF_SUBDIVISIONS 5

struct StringSubdivisionPair_
{
    const char *description;
    ClickSubdivision subdivision;
};

typedef struct StringSubdivisionPair_ StringSubdivisionPair;

static ClutterActor *drumscope = NULL;
static ClutterTimeline *timeline = NULL;
static gboolean metronome_running = FALSE;
static GtkWidget *subdivision_combo_box = NULL;
static GtkWidget *beats_spin_button = NULL;
static StringSubdivisionPair subdivision_pairs[NR_OF_SUBDIVISIONS] = { 
    { "One", SUB_ONE },
    { "Two", SUB_TWO },
    { "Swing", SUB_SWING },
    { "Three", SUB_THREE }, 
    { "Four", SUB_FOUR } };

static void
on_timeline_new_frame (ClutterTimeline *timeline, gint frame_num, gpointer data)
{
    guint32 current_tick = drum_io_poll ();
    ds_drumscope_set_cursor (DS_DRUMSCOPE (drumscope), current_tick);
}

static gboolean
on_start_button_clicked (GtkButton *button, gpointer user_data)
{
    metronome_running = !metronome_running;

    if (metronome_running)
    {
        // Update UI
        DsDrumtrack *drumtrack = ds_drumtrack_new ();
        drum_io_set_drumtrack (drumtrack);
        g_object_unref (drumtrack);
        ds_drumscope_set_drumtrack (DS_DRUMSCOPE (drumscope), drumtrack);
        ds_drumscope_reset (DS_DRUMSCOPE (drumscope));

        gtk_button_set_label (button, "Stop");
        gtk_widget_set_sensitive (subdivision_combo_box, FALSE);
        gtk_widget_set_sensitive (beats_spin_button, FALSE);

        // Start everything
        drum_io_start ();
        clutter_timeline_start (timeline);

    }
    else
    {
        // Update UI
        gtk_button_set_label (button, "Start");
        gtk_widget_set_sensitive (subdivision_combo_box, TRUE);
        gtk_widget_set_sensitive (beats_spin_button, TRUE);

        // Stop everything
        drum_io_stop ();
        clutter_timeline_stop (timeline);
    }

    return TRUE;
}

static gboolean
on_tempo_value_changed (GtkSpinButton *spin_button, gpointer user_data)
{
    gint tempo = gtk_spin_button_get_value_as_int (spin_button);
    drum_io_set_playback_tempo (tempo);

    return TRUE;
}

static gboolean
on_beat_config_changed (GtkWidget *widget, gpointer user_data)
{
    gint n_beats = gtk_spin_button_get_value_as_int (
            GTK_SPIN_BUTTON (beats_spin_button));
    gint subdivision_entry = gtk_combo_box_get_active (
            GTK_COMBO_BOX (subdivision_combo_box));
    ClickSubdivision subdivision = 
        subdivision_pairs[subdivision_entry].subdivision;

    ClickTrack *click_track = click_track_create (n_beats,
            subdivision);
    ds_drumscope_set_click_track (DS_DRUMSCOPE (drumscope), click_track);
    drum_io_set_click_track (click_track);

    return TRUE;
}

static gboolean
on_stage_size_changed (GtkWidget *widget, GdkEventConfigure *event,
        gpointer data)
{
    clutter_actor_set_size (drumscope, event->width, event->height);

    return FALSE;
}

GtkWidget*
create_main_window ()
{
    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), vbox);

    GtkWidget *hbox = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    GtkWidget *start_button = gtk_button_new_with_label ("Start");
    gtk_box_pack_end (GTK_BOX (hbox), start_button, FALSE, FALSE, 0);

    GtkWidget *tempo_label = gtk_label_new ("Tempo:");
    gtk_box_pack_start (GTK_BOX (hbox), tempo_label, FALSE, FALSE, 0);

    GtkAdjustment *tempo_adjustment = (GtkAdjustment *) gtk_adjustment_new (
            30.0, 30.0, 300.0, 1.0, 10.0, 0.0);
    GtkWidget *tempo_spin_button = gtk_spin_button_new (tempo_adjustment, 0.2,
            0);
    gtk_box_pack_start (GTK_BOX (hbox), tempo_spin_button, FALSE, FALSE, 0);

    GtkWidget *beats_label = gtk_label_new ("Beats:");
    gtk_box_pack_start (GTK_BOX (hbox), beats_label, FALSE, FALSE, 0);

    GtkAdjustment *beats_adjustment = (GtkAdjustment *) gtk_adjustment_new (
            4.0, 1.0, 10.0, 1.0, 1.0, 0.0);
    beats_spin_button = gtk_spin_button_new (beats_adjustment, 0.0,
            0);
    gtk_box_pack_start (GTK_BOX (hbox), beats_spin_button, FALSE, FALSE, 0);

    GtkWidget *subdivision_label = gtk_label_new ("Subdivision:");
    gtk_box_pack_start (GTK_BOX (hbox), subdivision_label, FALSE, FALSE, 0);

    subdivision_combo_box = gtk_combo_box_new_text ();
    for (int i = 0; i < NR_OF_SUBDIVISIONS; ++i)
    {
        gtk_combo_box_append_text (GTK_COMBO_BOX (subdivision_combo_box),
                subdivision_pairs[i].description);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (subdivision_combo_box), 0);
    gtk_box_pack_start (GTK_BOX (hbox), subdivision_combo_box, FALSE, FALSE, 0);

    GtkWidget *clutter_widget = gtk_clutter_embed_new ();
    gtk_box_pack_start (GTK_BOX (vbox), clutter_widget, TRUE, TRUE, 0);
    gtk_widget_set_size_request (clutter_widget, 320, 240);

    ClutterColor stage_color = { 0x00, 0x00, 0x00, 0xff };

    ClutterActor *stage = gtk_clutter_embed_get_stage (
            GTK_CLUTTER_EMBED (clutter_widget));
    clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);

    drumscope = ds_drumscope_new ();
    clutter_actor_set_size (drumscope, 320, 240);
    //clutter_actor_set_clip (drumscope, 0, 0, 640, 480);
    clutter_actor_set_position (drumscope, 0, 0);
    clutter_container_add_actor (CLUTTER_CONTAINER (stage), drumscope);

    // TODO: Move this somewhere?
    timeline = clutter_timeline_new (100);
    clutter_timeline_set_loop (timeline, TRUE);

    // Setup event handlers
    g_signal_connect (G_OBJECT (start_button), "clicked",
            G_CALLBACK (on_start_button_clicked), NULL);
    g_signal_connect (G_OBJECT (tempo_spin_button), "value-changed",
            G_CALLBACK (on_tempo_value_changed), NULL);

    g_signal_connect (G_OBJECT (beats_spin_button), "value-changed",
            G_CALLBACK (on_beat_config_changed), NULL);
    g_signal_connect (G_OBJECT (subdivision_combo_box), "changed",
            G_CALLBACK (on_beat_config_changed), NULL);

    g_signal_connect (G_OBJECT (timeline), "new-frame",
            G_CALLBACK (on_timeline_new_frame), NULL);

    g_signal_connect (G_OBJECT (clutter_widget), "configure_event",
            G_CALLBACK (on_stage_size_changed), NULL);

    // Setup default values, TODO: Do this in a better way
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (tempo_spin_button), 120.0);
    on_beat_config_changed (NULL, NULL);

    // Show, apperently the clutter actors must be shown after 
    // the clutter-gtk widget.
    gtk_widget_show_all (window);
    clutter_actor_show_all (stage);

    return window;
}

void
delete_main_window ()
{
    g_object_unref (timeline);
}

