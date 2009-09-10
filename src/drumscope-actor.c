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

#include <clutter/clutter.h>
#include <cogl/cogl.h>

G_DEFINE_TYPE (DsDrumscope, ds_drumscope, CLUTTER_TYPE_ACTOR);

#define DS_DRUMSCOPE_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
    DS_TYPE_DRUMSCOPE, DsDrumscopePrivate))

#define NR_OF_NOTE_LINES 5
#define CURSOR_MARGIN 48
#define LABEL_MARGIN 10
#define SCOPE_MARGIN 2

const char * const LABELS[NR_OF_NOTE_LINES] = {"C", "R", "H", "S", "K"};

struct _DsDrumscopePrivate
{
    guint32 cursor_tick;

    gboolean continous_scroll;
    guint32 visible_ticks;
    unsigned int n_visible_measures;
    guint32 cursor_margin;

    /* weak reference */
    DsDrumtrack *drumtrack;
    DrumTrackCursor first_visible_note;

    ClickTrack *click_track;  // TODO: Weak reference
    ClickTrackCursor first_visible_click;

    ClutterActor *labels[NR_OF_NOTE_LINES];

    // Cached values
    int note_lines_ycoord[NR_OF_NOTE_LINES];
    int max_label_width;
    guint32 start_tick;
    guint32 stop_tick;
};

static void
ds_drumscope_allocate (ClutterActor *actor,
                       const ClutterActorBox *box,
                       ClutterAllocationFlags flags)
{
    DsDrumscope *drumscope = DS_DRUMSCOPE (actor);
    DsDrumscopePrivate *priv = drumscope->priv;

    float height = box->y2 - box->y1;
    float y_step = height / (NR_OF_NOTE_LINES + 1);
    //printf ("height: %f, y_step: %f\n", height, y_step);

    float current_y = y_step;
    for (int i = 0; i < NR_OF_NOTE_LINES; ++i)
    {
        priv->note_lines_ycoord[i] = current_y;

        ClutterActor *child = priv->labels[i];

        // TODO: This should not be hardcoded
        ClutterActorBox child_box = {0, current_y - 13, 30, current_y + 10};
        clutter_actor_allocate (child, &child_box, 0);

        current_y += y_step;
    }

    // TODO: This should not be hardcoded
    priv->max_label_width = 20;

    // Chain up
    CLUTTER_ACTOR_CLASS (ds_drumscope_parent_class)->allocate (
            actor, box, flags);
}

static void
ds_drumscope_paint (ClutterActor *actor)
{
    DsDrumscope *drumscope = DS_DRUMSCOPE (actor);
    DsDrumscopePrivate *priv = drumscope->priv;

    for (int i = 0; i < NR_OF_NOTE_LINES; ++i)
    {
        ClutterActor *child = priv->labels[i];
        clutter_actor_paint (child);
    }

    ClutterGeometry geom;
    clutter_actor_get_geometry (actor, &geom);
    //printf ("Width: %d Height: %d\n", geom.width, geom.height);

    int scope_x = priv->max_label_width + LABEL_MARGIN;
    float scope_width = geom.width - scope_x - SCOPE_MARGIN;
    float x_factor = scope_width / priv->visible_ticks;

    CoglColor color;
    cogl_color_set_from_4ub (&color, 0x80, 0x80, 0xff, 0xff);
    cogl_set_source_color (&color);

    // Draw lines
    for (int i = 0; i < NR_OF_NOTE_LINES; ++i)
    {
        int current_y = priv->note_lines_ycoord[i];
        cogl_rectangle (scope_x, current_y, geom.width - SCOPE_MARGIN,
                current_y + 1);
    }

    // Draw click track bars
    if (priv->click_track != NULL)
    {
        ClickTrackCursor current_click = priv->first_visible_click;

        while (click_track_cursor_tick (current_click) < priv->stop_tick)
        {
            int rel_tick = click_track_cursor_tick (current_click) -
                priv->start_tick;
            int bar_x = scope_x + rel_tick * x_factor;
            int bar_width = 1;
            if (click_track_cursor_bar_type (current_click) ==
                    BAR_MEASURE_START)
            {
                bar_width = 3;
            }
            cogl_rectangle (bar_x, 0, bar_x + bar_width, geom.height);

            current_click = click_track_cursor_next_click (current_click);
        }
    }

    // Draw cursor
    CoglColor cursor_color;
    cogl_color_set_from_4ub (&cursor_color, 0x80, 0xff, 0x80, 0xff);
    cogl_set_source_color (&cursor_color);
    int cursor_x = scope_x + (priv->cursor_tick - priv->start_tick) * x_factor;
    cogl_rectangle (cursor_x, 0, cursor_x + 1, geom.height);

    // Draw notes
    CoglColor note_color;
    cogl_color_set_from_4ub (&note_color, 0xff, 0xff, 0xff, 0xff);
    cogl_set_source_color (&note_color);

    if (priv->drumtrack != NULL)
    {
        // Start from beginning of drum track if we have reached the end.
        DrumTrackCursor cursor = priv->first_visible_note;
        if (ds_drumtrack_cursor_at_end (cursor))
        {
            cursor = ds_drumtrack_begin (priv->drumtrack);
        }

        // Find the first visible note and save the cursor
        while (!ds_drumtrack_cursor_at_end (cursor))
        {
            DrumNote *note = ds_drumtrack_cursor_data (cursor);
            if (note->tick >= priv->start_tick)
            {
                break;
            }
            cursor = ds_drumtrack_cursor_next (cursor);
        }
        priv->first_visible_note = cursor;

        while (!ds_drumtrack_cursor_at_end (cursor))
        {
            DrumNote *note = ds_drumtrack_cursor_data (cursor);

            if (note->tick >= priv->stop_tick)
            {
                break;
            }

            int current_x = scope_x + (note->tick - priv->start_tick) * 
                x_factor;
            int current_y = priv->note_lines_ycoord[note->drum];
            cogl_rectangle (current_x - 4, current_y - 4,
                    current_x + 5, current_y + 5);

            cursor = ds_drumtrack_cursor_next (cursor);
        }
    }
}


static void
ds_drumscope_map (ClutterActor *actor)
{
    DsDrumscope *drumscope = DS_DRUMSCOPE (actor);
    DsDrumscopePrivate *priv = drumscope->priv;

    // Chain up
    CLUTTER_ACTOR_CLASS (ds_drumscope_parent_class)->map (actor);

    for (int i = 0; i < NR_OF_NOTE_LINES; ++i)
    {
        clutter_actor_map (priv->labels[i]);
    }
}

static void
ds_drumscope_unmap (ClutterActor *actor)
{
    DsDrumscope *drumscope = DS_DRUMSCOPE (actor);
    DsDrumscopePrivate *priv = drumscope->priv;

    for (int i = 0; i < NR_OF_NOTE_LINES; ++i)
    {
        clutter_actor_unmap (priv->labels[i]);
    }

    // Chain up
    CLUTTER_ACTOR_CLASS (ds_drumscope_parent_class)->unmap (actor);
}

static void
ds_drumscope_finalize (GObject *object)
{
    DsDrumscope *drumscope = DS_DRUMSCOPE (object);
    DsDrumscopePrivate *priv = drumscope->priv;

    // Chain up
    G_OBJECT_CLASS (ds_drumscope_parent_class)->finalize (object);
}

static void
ds_drumscope_dispose (GObject *object)
{
    DsDrumscope *drumscope = DS_DRUMSCOPE (object);
    DsDrumscopePrivate *priv = drumscope->priv;

    for (int i = 0; i < NR_OF_NOTE_LINES; ++i)
    {
        if (priv->labels[i] != NULL)
        {
            clutter_actor_unparent (priv->labels[i]);
            priv->labels[i] = NULL;
        }
    }

    // Chain up
    G_OBJECT_CLASS (ds_drumscope_parent_class)->dispose (object);
}

static void
ds_drumscope_class_init (DsDrumscopeClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

    actor_class->paint = ds_drumscope_paint;
    actor_class->allocate = ds_drumscope_allocate;
    actor_class->map = ds_drumscope_map;
    actor_class->unmap = ds_drumscope_unmap;

    gobject_class->finalize = ds_drumscope_finalize;
    gobject_class->dispose = ds_drumscope_dispose;

    g_type_class_add_private (gobject_class, sizeof (DsDrumscopePrivate));
}

static void
ds_drumscope_init (DsDrumscope *drumscope)
{
    DsDrumscopePrivate *priv;

    drumscope->priv = priv = DS_DRUMSCOPE_GET_PRIVATE (drumscope);

    priv->continous_scroll = FALSE;
    priv->n_visible_measures = 2;
    priv->cursor_margin = CURSOR_MARGIN;
    priv->visible_ticks = 96 * 4 + priv->cursor_margin;

    priv->cursor_tick = 0;
    priv->start_tick = 0;
    priv->stop_tick = priv->visible_ticks;

    priv->drumtrack = NULL;
    priv->click_track = NULL;

    ClutterColor text_color = {0xff, 0xff, 0xff, 0xff};
    for (int i = 0; i < NR_OF_NOTE_LINES; ++i)
    {
        ClutterActor *child = clutter_text_new ();
        clutter_text_set_text (CLUTTER_TEXT (child), LABELS[i]);
        clutter_text_set_color (CLUTTER_TEXT (child), &text_color);
        clutter_text_set_font_name (CLUTTER_TEXT (child), "Sans 14");
        clutter_text_set_editable (CLUTTER_TEXT (child), FALSE);
        clutter_actor_set_parent (child, CLUTTER_ACTOR (drumscope));
        clutter_actor_show (child);

        priv->labels[i] = child;
    }
}


/**
 * Creates a new drumscope actor.
 */
ClutterActor*
ds_drumscope_new (void)
{
    return g_object_new (DS_TYPE_DRUMSCOPE, NULL);
}

/**
 * Sets the click track for the drumscope. Ownership of the click track is not
 * taken and must be unset before it is deleted.
 */
void
ds_drumscope_set_click_track (DsDrumscope *drumscope,
        ClickTrack *click_track)
{
    // TODO: click track should be a weak reference
    DsDrumscopePrivate *priv = drumscope->priv;

    priv->click_track = click_track;
    // Setting the click track currently implies non-contiuous scrolling
    priv->continous_scroll = FALSE;

    // TODO: Handle this better
    ds_drumscope_reset (drumscope);
}

static void
on_drumtrack_delete (gpointer data, GObject *prev_address)
{
    DsDrumscope *drumscope = DS_DRUMSCOPE (data);
    DsDrumscopePrivate *priv = drumscope->priv;

    priv->drumtrack = NULL;
}

/**
 * Sets the drumtrack that the drumscope should show. The drumscope
 * only holds a weak reference to the drumtrack.
 */
void
ds_drumscope_set_drumtrack (DsDrumscope *drumscope, DsDrumtrack *new_drumtrack)
{
    DsDrumscopePrivate *priv = drumscope->priv;

    priv->drumtrack = new_drumtrack;
    g_object_weak_ref (new_drumtrack, on_drumtrack_delete, drumscope);

    priv->first_visible_note = ds_drumtrack_begin (new_drumtrack);
}

/**
 * Sets drumscope cursor at tick. Currently the tick must always be increasing.
 */
void
ds_drumscope_set_cursor (DsDrumscope *drumscope, const guint32 tick)
{
    DsDrumscopePrivate *priv = drumscope->priv;

    g_assert (tick >= priv->cursor_tick);

    priv->cursor_tick = tick;

    if (priv->continous_scroll)
    {
        priv->stop_tick = priv->cursor_tick + priv->cursor_margin;
        priv->start_tick = priv->stop_tick - priv->visible_ticks;

        // Update cursor to first visible click
        while (click_track_cursor_tick (priv->first_visible_click)
                < priv->start_tick)
        {
            priv->first_visible_click = click_track_cursor_next_click (
                    priv->first_visible_click);
        }
    }
    else
    {
        g_assert (priv->click_track != NULL);

        if (priv->cursor_tick > priv->stop_tick)
        {
            priv->first_visible_click =
                click_track_cursor_next_measure (priv->first_visible_click);
        }

        ClickTrackCursor cursor = priv->first_visible_click;
        priv->start_tick = click_track_cursor_tick (cursor);

        for (int i = 0; i < priv->n_visible_measures; ++i)
        {
            cursor = click_track_cursor_next_measure (cursor);
        }

        // +1 to include the bar of the next measure
        priv->stop_tick = click_track_cursor_tick (cursor) + 1;
        priv->visible_ticks = priv->stop_tick - priv->start_tick;
    }

    clutter_actor_queue_redraw (CLUTTER_ACTOR (drumscope));
}

/**
 * Resets drumscope, moving cursor to position 0.
 */
void
ds_drumscope_reset (DsDrumscope *drumscope)
{
    DsDrumscopePrivate *priv = drumscope->priv;

    priv->cursor_tick = 0;

    if (priv->click_track != NULL)
    {
        priv->first_visible_click = click_track_begin (priv->click_track);
    }

    ds_drumscope_set_cursor (drumscope, 0);
}

