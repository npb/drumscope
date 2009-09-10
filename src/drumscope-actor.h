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

#ifndef __DRUMSCOPE_ACTOR_H__
#define __DRUMSCOPE_ACTOR_H__

#include "drum-track.h"
#include "click-track.h"

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

/*
 * Type macros
 */
#define DS_TYPE_DRUMSCOPE (ds_drumscope_get_type())
#define DS_DRUMSCOPE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), DS_TYPE_DRUMSCOPE, DsDrumscope))
#define DS_IS_DRUMSCOPE(obj) \
    (G_TYPE_INSTANCE_TYPE ((obj), DS_TYPE_DRUMSCOPE))
#define DS_DRUMSCOPE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), DS_TYPE_DRUMSCOPE, DsDrumscopeClass))
#define DS_IS_DRUMSCOPE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), DS_TYPE_DRUMSCOPE))
#define DS_DRUMSCOPE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), DS_TYPE_DRUMSCOPE, DsDrumscopeClass))

typedef struct _DsDrumscope DsDrumscope;
typedef struct _DsDrumscopeClass DsDrumscopeClass;
typedef struct _DsDrumscopePrivate DsDrumscopePrivate;

struct _DsDrumscope
{
    ClutterActor parent;

    /*< private >*/
    DsDrumscopePrivate *priv;
};

struct _DsDrumscopeClass
{
    ClutterActorClass parent_class;
};

GType ds_drumscope_get_type (void) G_GNUC_CONST;

ClutterActor *ds_drumscope_new (void);
void ds_drumscope_set_click_track (DsDrumscope *drumscope,
        ClickTrack *click_track);
void ds_drumscope_set_drumtrack (DsDrumscope *drumscope, 
        DsDrumtrack *drumtrack);
void ds_drumscope_set_cursor (DsDrumscope *drumscope, unsigned int ticks);
void ds_drumscope_reset (DsDrumscope *drumscope);

G_END_DECLS

#endif // __DRUMSCOPE_ACTOR_H__

