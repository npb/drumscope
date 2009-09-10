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

#include "click-track.h"

typedef struct Click_ Click;

struct Click_
{
    unsigned int tick;
    ClickType type;
    ClickBarType bar_type;
};

struct TrackMeasure_
{
    int n_clicks;  // Nr of clicks in measure
    Click *clicks;  // Array of clicks, increasing tick
    unsigned int length;  // Length of measure in ticks
};

struct ClickTrack_
{
    GList *measures;
};


ClickTrack *click_track_create (
        int beats_per_measure, 
        ClickSubdivision subdivision)
{
    ClickTrack *click_track = g_malloc (sizeof (ClickTrack));
    click_track->measures = NULL;

    int n_subclicks = 0;
    switch (subdivision)
    {
        case SUB_ONE:
            n_subclicks = 1;
            break;
        case SUB_TWO:
            n_subclicks = 2;
            break;
        case SUB_SWING:
            n_subclicks = 2;
            break;
        case SUB_THREE:
            n_subclicks = 3;
            break;
        case SUB_FOUR:
            n_subclicks = 4;
            break;
    }

    TrackMeasure *measure = g_malloc (sizeof (TrackMeasure));
    measure->n_clicks = beats_per_measure * n_subclicks;
    measure->clicks = g_malloc (sizeof (Click) * beats_per_measure *
            n_subclicks);
    measure->length = beats_per_measure * 96;

    unsigned int subclick_step = 96 / n_subclicks;
    if (subdivision == SUB_SWING)
    {
        subclick_step = 96 / 3;
    }
    unsigned int tick = 0;

    for (int i = 0; i < measure->n_clicks; ++i)
    {
        measure->clicks[i].tick = tick;

        ClickType type = CLICK_NORMAL;
        ClickBarType bar_type = BAR_NORMAL;
        if (tick == 0)
        {
            type = CLICK_ACCENTED;
            bar_type = BAR_MEASURE_START;
        }
        else if (tick % 96 != 0)
        {
            type = CLICK_WEAK;
            bar_type = BAR_SUB;
        }
        measure->clicks[i].type = type;
        measure->clicks[i].bar_type = bar_type;

        if (subdivision == SUB_SWING && tick % 96 == 0)
        {
            tick += subclick_step * 2;
        }
        else
        {
            tick += subclick_step;
        }
    }

    click_track->measures = g_list_append (click_track->measures, measure);

    return click_track;
}

void click_track_free (ClickTrack *click_track)
{
    GList *node = click_track->measures;
    while (node != NULL)
    {
        TrackMeasure *measure = node->data;
        g_free (measure->clicks);
        g_free (measure);

        node = node->next;
    }

    g_list_free (click_track->measures);

    g_free (click_track);
}

ClickTrackCursor click_track_begin (ClickTrack *click_track)
{
    ClickTrackCursor cursor = { click_track: click_track, measure_start_tick: 0,
        measure_node: click_track->measures, n_click: 0 };

    return cursor;
}

ClickTrackCursor click_track_cursor_next_click (ClickTrackCursor cursor)
{
    TrackMeasure *cur_measure = cursor.measure_node->data;

    ClickTrackCursor new_cursor;

    if (cursor.n_click + 1 < cur_measure->n_clicks)
    {
        new_cursor.click_track = cursor.click_track;
        new_cursor.measure_start_tick = cursor.measure_start_tick;
        new_cursor.measure_node = cursor.measure_node;
        new_cursor.n_click = cursor.n_click + 1;
    }
    else
    {
        new_cursor = click_track_cursor_next_measure (cursor);
    }

    return new_cursor;
}

ClickTrackCursor click_track_cursor_next_measure (ClickTrackCursor cursor)
{
    TrackMeasure *cur_measure = cursor.measure_node->data;

    ClickTrackCursor new_cursor;
    new_cursor.click_track = cursor.click_track;
    new_cursor.measure_start_tick = cursor.measure_start_tick +
        cur_measure->length;
    new_cursor.n_click = 0;
    if (cursor.measure_node->next != NULL)
    {
        new_cursor.measure_node = cursor.measure_node->next;
    }
    else
    {
        new_cursor.measure_node = cursor.click_track->measures;
    }

    return new_cursor;
}

unsigned int click_track_cursor_measure_length (ClickTrackCursor cursor)
{
    TrackMeasure *measure = cursor.measure_node->data;
    return measure->length;
}

unsigned int click_track_cursor_tick (ClickTrackCursor cursor)
{
    TrackMeasure *measure = cursor.measure_node->data;
    return cursor.measure_start_tick + measure->clicks[cursor.n_click].tick;
}

ClickType click_track_cursor_click_type (ClickTrackCursor cursor)
{
    TrackMeasure *measure = cursor.measure_node->data;
    return measure->clicks[cursor.n_click].type;
}

ClickBarType click_track_cursor_bar_type (ClickTrackCursor cursor)
{
    TrackMeasure *measure = cursor.measure_node->data;
    return measure->clicks[cursor.n_click].bar_type;
}

