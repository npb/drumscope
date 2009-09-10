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

#include "drum-io.h"

#include <glib.h>
#include <alsa/asoundlib.h>

#define MIDI_NOP 0

#define OUTPUT_MARGIN 96

static snd_seq_t *seq = NULL;
static int out_port_id = -1;
static int queue_id = -1;

static DsDrumtrack *drumtrack = NULL;
static ClickTrack *g_click_track = NULL;
static ClickTrackCursor g_cursor;
static gboolean running = FALSE;

static inline DrumType
midi_note_to_drum (unsigned char midi_note)
{
    switch (midi_note)
    {
        case 26:
        case 46:
            return DRUM_HIHAT;
        case 36:
            return DRUM_KICK;
        case 38:
            return DRUM_SNARE;

        default:
            printf ("Unknown note: %d\n", midi_note);
            return DRUM_CRASH;
    }
}

static inline int 
click_type_to_velocity (ClickType type)
{
    switch (type)
    {
        case CLICK_NORMAL:
            return 60;
        case CLICK_ACCENTED:
            return 80;
        case CLICK_WEAK:
            return 40;
    }

    return 0;
}

static gboolean
data_pending (void)
{
    return snd_seq_event_input_pending (seq, TRUE) != 0;
}

static DrumNote*
get_note (void)
{
#if !MIDI_NOP
    snd_seq_event_t *ev;
    int err;

    err = snd_seq_event_input (seq, &ev);
    assert (err >= 0);

    if (ev->type == SND_SEQ_EVENT_NOTEON)
    {
        DrumNote *note = g_malloc (sizeof (DrumNote));
        note->drum = midi_note_to_drum (ev->data.note.note);
        note->tick = ev->time.tick;
        note->velocity = ev->data.note.velocity << (32 - 7);

        return note;
    }

#endif
    return NULL;
}

static guint32
get_current_tick (void)
{
    snd_seq_queue_status_t *status;
    snd_seq_queue_status_alloca (&status);
    snd_seq_get_queue_status (seq, queue_id, status);
    return snd_seq_queue_status_get_tick_time (status);
}

static gboolean
put_click (const DrumClick *click)
{
    snd_seq_event_t ev;
    snd_seq_ev_clear (&ev);

    snd_seq_ev_set_source (&ev, out_port_id);
    snd_seq_ev_set_subs (&ev);
    snd_seq_ev_schedule_tick (&ev, queue_id, 0, click->tick);
    snd_seq_ev_set_note (&ev, 10, 24, click->velocity, 48);

    int err = snd_seq_event_output_direct (seq, &ev);
    return err >= 0;
}

static void 
playback_poll (guint32 current_tick)
{
    if (g_click_track != NULL)
    {
        gboolean buffer_full = FALSE;
        while ((click_track_cursor_tick (g_cursor) < 
                    (current_tick + OUTPUT_MARGIN)) && !buffer_full)
        {
            guint32 tick = click_track_cursor_tick (g_cursor);
            int velocity = click_type_to_velocity (
                    click_track_cursor_click_type (g_cursor));
            DrumClick dclick = { velocity: velocity, tick: tick };

            //printf ("outputting: %d, %d\n", velocity, tick);
            if (put_click (&dclick))
            {
                g_cursor = click_track_cursor_next_click (g_cursor);
            }
            else
            {
                printf ("Buffer full\n");
                buffer_full = TRUE;
            }
        }
    }
}

/**
 * Initiates the drum I/O.
 */
void drum_io_init (int input_client, int input_port, int output_client,
        int output_port)
{
    int err;

    err = snd_seq_open (&seq, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK);
    assert (err >= 0);

    snd_seq_set_client_name (seq, "drumscope");
    int client_id = snd_seq_client_id (seq);

    out_port_id = snd_seq_create_simple_port (seq, "output",
            SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
            SND_SEQ_PORT_TYPE_MIDI_GENERIC);
    assert (out_port_id >= 0);

    int in_port_id = snd_seq_create_simple_port (seq, "input",
            SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
            SND_SEQ_PORT_TYPE_MIDI_GENERIC);
    assert (in_port_id >= 0);

    queue_id = snd_seq_alloc_queue (seq);
    assert (queue_id >= 0);

#if !MIDI_NOP
    err = snd_seq_connect_to (seq, out_port_id, output_client, output_port);
    assert (err >= 0);

    snd_seq_addr_t sender = { client: input_client, port: input_port };
    snd_seq_addr_t dest = { client: client_id, port: in_port_id };
    snd_seq_port_subscribe_t *subs;
    snd_seq_port_subscribe_alloca (&subs);
    snd_seq_port_subscribe_set_sender (subs, &sender);
    snd_seq_port_subscribe_set_dest (subs, &dest);
    snd_seq_port_subscribe_set_queue (subs, queue_id);
    snd_seq_port_subscribe_set_time_update (subs, 1);
    err = snd_seq_subscribe_port (seq, subs);
    assert (err >= 0);
#endif

}

/**
 * Sets the drumtrack that will have notes added to it when polling. Must not
 * be called when drum I/O is running.
 */
void
drum_io_set_drumtrack (DsDrumtrack *new_drumtrack)
{
    g_assert (!running);

    if (drumtrack != NULL)
    {
        g_object_unref (drumtrack);
    }

    drumtrack = new_drumtrack;
    g_object_ref (drumtrack);
}

/**
 * Sets the click track to play. Must not be called when drum I/O is running.
 */
void
drum_io_set_click_track (ClickTrack *click_track)
{
    g_assert (!running);

    if (g_click_track != NULL)
    {
        click_track_free (g_click_track);
    }
    g_click_track = click_track;
}

/**
 * Sets the playback tempo. May be called while drum I/O is running.
 */
void
drum_io_set_playback_tempo (int bpm)
{
    g_assert (bpm > 0);
    g_assert (bpm < 350);

    unsigned int tempo = 60 * 1000000 / bpm;

    int err = snd_seq_change_queue_tempo (seq, queue_id, tempo, NULL);
    assert (err >= 0);
    err = snd_seq_drain_output (seq);
    assert (err >= 0);
}

/**
 * This funtion should be called periodically to handle drum I/O. 
 * Plays the click track if it is set. 
 * Checks for new notes. Any new notes are added to the drumtrack previously
 * set by drum_io_set_drumtrack().
 */
guint32
drum_io_poll (void)
{
    while (data_pending ())
    {
        DrumNote *note = get_note ();
        if (note != NULL)
        {
            if (drumtrack != NULL)
            {
                ds_drumtrack_append_note (drumtrack, note);
            }
            else
            {
                // TODO: This sucks
                g_free (note);
            }
        }
    }

    guint32 current_tick = get_current_tick ();
    playback_poll (current_tick);

    return current_tick;
}

/**
 * Starts drum I/O.
 */
void
drum_io_start (void)
{
    g_assert (!running);

    int err;

    if (g_click_track != NULL)
    {
        g_cursor = click_track_begin (g_click_track);
    }

    err = snd_seq_start_queue (seq, queue_id, NULL);
    assert (err >= 0);
    err = snd_seq_drain_output (seq);
    assert (err >= 0);

    running = TRUE;
}

/**
 * Stops drum I/O.
 */
void
drum_io_stop (void)
{
    g_assert (running);

    int err;

    err = snd_seq_stop_queue (seq, queue_id, NULL);
    assert (err >= 0);
    err = snd_seq_drain_output (seq);
    assert (err >= 0);
    err = snd_seq_drop_output (seq);
    assert (err >= 0);

    running = FALSE;
}

