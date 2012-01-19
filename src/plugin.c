/*
 * weechat-eventc - Weechat plugin client for eventd
 *
 * Copyright © 2011-2012 Quentin "Sardem FF7" Glidic
 *
 * This file is part of weechat-eventc.
 *
 * weechat-eventc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * weechat-eventc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with weechat-eventc. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <glib.h>
#include <glib-compat.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <weechat-plugin.h>

WEECHAT_PLUGIN_NAME("eventc");
WEECHAT_PLUGIN_DESCRIPTION("Client for eventd");
WEECHAT_PLUGIN_AUTHOR("Quentin Glidic <sardemff7+weechat@sardemff7.net>");
WEECHAT_PLUGIN_VERSION("0.1");
WEECHAT_PLUGIN_LICENSE("GPL3");

extern struct t_weechat_plugin *weechat_plugin;

void eventd_weechat_plugin_init(gchar *argv[], gint argc);
void eventd_weechat_plugin_disconnect(GCallback callback);
void eventd_weechat_plugin_end();

static GMainLoop* loop;
static GThread* thread;

static void *
run_loop()
{
    g_main_loop_run(loop);
    return NULL;
}

static void
quit_loop()
{
    g_main_loop_quit(loop);
}

int
weechat_plugin_init(struct t_weechat_plugin *plugin, gint argc, gchar *argv[])
{
    weechat_plugin = plugin;
    g_type_init();
    loop = g_main_loop_new(NULL, TRUE);
    eventd_weechat_plugin_init(argv, argc);
    thread = g_thread_new("weechat-eventc main loop", run_loop, NULL);
    return WEECHAT_RC_OK;
}

int
weechat_plugin_end(struct t_weechat_plugin *plugin)
{
    eventd_weechat_plugin_disconnect(quit_loop);
    g_thread_join(thread);
    eventd_weechat_plugin_end();
    return WEECHAT_RC_OK;
}
