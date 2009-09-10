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

#ifndef __DRUM_IO_H__
#define __DRUM_IO_H__

#include "drum-track.h"
#include "click-track.h"
#include <glib.h>

typedef struct _DrumClick DrumClick;

struct _DrumClick
{
    unsigned int tick;
    unsigned int velocity;
};

void drum_io_init (int input_client, int input_port, int output_client,
        int output_port);
void drum_io_set_midi_to_drum_map ();
void drum_io_set_click_to_midi_map ();

void drum_io_set_drumtrack (DsDrumtrack *new_drumtrack);
void drum_io_set_click_track (ClickTrack *click_track);
void drum_io_set_playback_tempo (int bpm);

void drum_io_start (void);
void drum_io_stop (void);
guint32 drum_io_poll (void);

#endif // __DRUM_IO_H__

