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

#ifndef __CLICK_TRACK_H__
#define __CLICK_TRACK_H__

#include <glib.h>

typedef struct ClickTrack_ ClickTrack;
typedef struct TrackMeasure_ TrackMeasure;
typedef struct ClickTrackCursor_ ClickTrackCursor;

typedef enum ClickSubdivision_ ClickSubdivision;
typedef enum ClickType_ ClickType;
typedef enum ClickBarType_ ClickBarType;

struct ClickTrackCursor_
{
    /* Private */
    ClickTrack *click_track;
    unsigned int measure_start_tick;
    GList *measure_node;
    int n_click;
};

enum ClickSubdivision_ { SUB_ONE, SUB_TWO, SUB_SWING, SUB_THREE, SUB_FOUR };
enum ClickType_ { CLICK_NORMAL, CLICK_ACCENTED, CLICK_WEAK };
enum ClickBarType_ { BAR_MEASURE_START, BAR_NORMAL, BAR_SUB, BAR_NONE };

ClickTrack *click_track_create (
        int beats_per_measure,
        ClickSubdivision subdivision);
void click_track_free (ClickTrack *click_track);
ClickTrackCursor click_track_begin (ClickTrack *click_track);

ClickTrackCursor click_track_cursor_next_click (ClickTrackCursor cursor);
ClickTrackCursor click_track_cursor_next_measure (ClickTrackCursor cursor);
unsigned int click_track_cursor_measure_length (ClickTrackCursor cursor);
unsigned int click_track_cursor_tick (ClickTrackCursor cursor);
ClickType click_track_cursor_click_type (ClickTrackCursor cursor);
ClickBarType click_track_cursor_bar_type (ClickTrackCursor cursor);

#endif // __CLICK_TRACK_H__

