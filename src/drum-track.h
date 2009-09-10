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

#ifndef __DRUM_TRACK_H__
#define __DRUM_TRACK_H__

#include <glib-object.h>
#include <glib.h>

G_BEGIN_DECLS

/*
 * Type macros
 */
#define DS_TYPE_DRUMTRACK (ds_drumtrack_get_type())
#define DS_DRUMTRACK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), DS_TYPE_DRUMTRACK, DsDrumtrack))
#define DS_IS_DRUMTRACK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DS_TYPE_DRUMTRACK))
#define DS_DRUMTRACK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), DS_TYPE_DRUMTRACK, DsDrumtrackClass))
#define DS_IS_DRUMTRACK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), DS_TYPE_DRUMTRACK))
#define DS_DRUMTRACK_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), DS_TYPE_DRUMTRACK, DsDrumtrackClass))

typedef struct _DsDrumtrack DsDrumtrack;
typedef struct _DsDrumtrackClass DsDrumtrackClass;
struct _DsDrumtrack
{
    GObject parent_instance;

    /*< private >*/
    GQueue *notes;
};

struct _DsDrumtrackClass
{
    GObjectClass parent_class;
};

enum _DrumType
{ 
    DRUM_CRASH,
    DRUM_RIDE,
    DRUM_HIHAT,
    DRUM_SNARE,
    DRUM_KICK
};
typedef enum _DrumType DrumType;

struct _DrumNote
{
    guint32 tick;
    gint32 velocity;
    DrumType drum;
};
typedef struct _DrumNote DrumNote;

struct _DrumTrackCursor
{
    /* Private */
    GList *current_note;
};
typedef struct _DrumTrackCursor DrumTrackCursor;

GType ds_drumtrack_get_type (void) G_GNUC_CONST;

DsDrumtrack *ds_drumtrack_new (void);
void ds_drumtrack_append_note (DsDrumtrack *drum_track, DrumNote *note);

DrumTrackCursor ds_drumtrack_begin (DsDrumtrack *drum_track);
DrumTrackCursor ds_drumtrack_cursor_next (DrumTrackCursor cursor);
gboolean ds_drumtrack_cursor_at_end (DrumTrackCursor cursor);
DrumNote *ds_drumtrack_cursor_data (DrumTrackCursor cursor);

G_END_DECLS

#endif // __DRUM_TRACK_H__

