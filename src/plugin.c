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

typedef struct {
    GSourceFunc callback;
    gpointer user_data;
} WeechatEventcWaiterData;

struct t_weechat_plugin *weechat_plugin = NULL;

WEECHAT_PLUGIN_NAME("eventc");
WEECHAT_PLUGIN_DESCRIPTION("Client for eventd");
WEECHAT_PLUGIN_AUTHOR("Quentin Glidic <sardemff7+weechat@sardemff7.net>");
WEECHAT_PLUGIN_VERSION("0.1");
WEECHAT_PLUGIN_LICENSE("GPL3");

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

static gpointer pending = NULL;
static GMutex *mutex;
#if GLIB_CHECK_VERSION(2,32,0)
static GMutex mutex_;
#endif /* GLIB_CHECK_VERSION(2,32,0) */
static GQueue *queue;

gboolean
eventd_weechat_plugin_lock(GSourceFunc callback, gpointer user_data)
{
    gboolean r = FALSE;
    g_mutex_lock(mutex);
    if ( ( pending == NULL ) || ( pending == user_data ) )
    {
        r = TRUE;
        pending = user_data;
    }
    else
    {
        WeechatEventcWaiterData *data;

        data = g_slice_new(WeechatEventcWaiterData);
        data->callback = callback;
        data->user_data = user_data;

        g_queue_push_tail(queue, data);
    }
    g_mutex_unlock(mutex);
    return r;
}

void
eventd_weechat_plugin_unlock()
{
    g_mutex_lock(mutex);
    pending = NULL;
    WeechatEventcWaiterData *data;
    data = g_queue_pop_head(queue);
    if ( data != NULL )
    {
        pending = data->user_data;
        g_idle_add(data->callback, data->user_data);
        g_slice_free(WeechatEventcWaiterData, data);
    }
    g_mutex_unlock(mutex);
}

int
weechat_plugin_init(struct t_weechat_plugin *plugin, gint argc, gchar *argv[])
{
    weechat_plugin = plugin;
    g_type_init();
#if GLIB_CHECK_VERSION(2,32,0)
    g_mutex_init(&mutex_);
    mutex = &mutex_;
#else /* ! GLIB_CHECK_VERSION(2,32,0) */
    mutex = g_mutex_new();
#endif /* ! GLIB_CHECK_VERSION(2,32,0) */
    queue = g_queue_new();
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
#if GLIB_CHECK_VERSION(2,32,0)
    g_queue_free_full(queue, g_free);
    g_mutex_clear(&mutex_);
#else /* ! GLIB_CHECK_VERSION(2,32,0) */
    g_queue_foreach(queue, (GFunc)g_free, NULL);
    g_queue_free(queue);
    g_mutex_free(mutex);
#endif /* ! GLIB_CHECK_VERSION(2,32,0) */
    eventd_weechat_plugin_end();
    return WEECHAT_RC_OK;
}
