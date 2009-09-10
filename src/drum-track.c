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

#include "drum-track.h"

G_DEFINE_TYPE (DsDrumtrack, ds_drumtrack, G_TYPE_OBJECT);

#define CHANGED_SIGNAL 0
#define NO_OF_SIGNALS 1
// TODO: Should this be in the class struct?
static guint drumtrack_signals[NO_OF_SIGNALS];

static void
ds_drumtrack_finalize (GObject *object)
{
    DsDrumtrack *drumtrack = DS_DRUMTRACK (object);

    GList *node = drumtrack->notes->head;
    while (node != NULL)
    {
        g_free (node->data);
        node = node->next;
    }

    g_queue_free (drumtrack->notes);

    // Chain up
    G_OBJECT_CLASS (ds_drumtrack_parent_class)->finalize (object);
}

static void
ds_drumtrack_init (DsDrumtrack *object)
{
    object->notes = g_queue_new ();
}

static void
ds_drumtrack_class_init (DsDrumtrackClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = ds_drumtrack_finalize;

    drumtrack_signals[CHANGED_SIGNAL] = g_signal_newv ("changed",
            G_TYPE_FROM_CLASS (gobject_class),
            G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
            NULL,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE,
            0,
            NULL);
}


/**
 * Creates a new empty drumtrack.
 */
DsDrumtrack*
ds_drumtrack_new (void)
{
    return g_object_new (DS_TYPE_DRUMTRACK, NULL);
}

/**
 * Appends a note to the drumtrack. The drumtrack takes ownership of the note.
 * Emits "changed" signal.
 */
void
ds_drumtrack_append_note (DsDrumtrack *drumtrack, DrumNote *note)
{
    DrumNote *prev_note = g_queue_peek_tail (drumtrack->notes);
    if (prev_note != NULL)
    {
        g_assert (prev_note->tick <= note->tick);
    }

    g_queue_push_tail (drumtrack->notes, note);

    g_signal_emit (drumtrack, drumtrack_signals[CHANGED_SIGNAL], 0);
}

/**
 * Returns a cursor to the first note of the track.
 */
DrumTrackCursor
ds_drumtrack_begin (DsDrumtrack *drumtrack)
{
    DrumTrackCursor cursor = { current_note: drumtrack->notes->head };

    return cursor;
}

/**
 * Advances the cursor to the next note in the track.
 */
DrumTrackCursor
ds_drumtrack_cursor_next (DrumTrackCursor cursor)
{
    g_assert (cursor.current_note != NULL);

    DrumTrackCursor new_cursor = { current_note: cursor.current_note->next };

    return new_cursor;
}

/**
 * Returns true if the cursor is at the end of the track. The end is one
 * note past the last note and is not valid to dereference.
 */
gboolean
ds_drumtrack_cursor_at_end (DrumTrackCursor cursor)
{
    return cursor.current_note == NULL;
}

/**
 * Returns the note that the cursor points at.
 */
DrumNote*
ds_drumtrack_cursor_data (DrumTrackCursor cursor)
{
    g_assert (cursor.current_note != NULL);

    return cursor.current_note->data;
}

