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

#include <stdlib.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <clutter/clutter.h>

#include "drum-io.h"
#include "main-window.h"

static gint input_client = 20;
static gint input_port = 1;
static gint output_client = 20;
static gint output_port = 0;

static GOptionEntry option_entries[] =
{
    { "input-client", 0, 0, G_OPTION_ARG_INT, &input_client,
        "Alsa client id to use for midi input", "id"},
    { "input-port", 0, 0, G_OPTION_ARG_INT, &input_port,
        "Port id to use for midi input", "port"},
    { "output-client", 0, 0, G_OPTION_ARG_INT, &output_client,
        "Alsa client id to use for midi output", "id"},
    { "output-port", 0, 0, G_OPTION_ARG_INT, &output_port,
        "Port id to use for midi output", "port"},
};

int
main (int argc, char *argv[])
{
    GError *error = NULL;
    GOptionContext *context;
    context = g_option_context_new ("- A graphical metronome for drummers");
    g_option_context_add_main_entries (context, option_entries, NULL);
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
    g_option_context_add_group (context, clutter_get_option_group ());

    if (!g_option_context_parse (context, &argc, &argv, &error))
    {
        g_print ("option parsing failed: %s\n", error->message);
        exit (1);
    }

    drum_io_init (input_client, input_port, output_client, output_port);

    GtkWidget *window = create_main_window ();

    g_signal_connect (window, "hide",
            G_CALLBACK (gtk_main_quit), NULL);

    gtk_main();

    delete_main_window();

    return EXIT_SUCCESS;
}

