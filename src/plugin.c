/*
 * weechat-eventc - Weechat plugin client for eventd
 *
 * Copyright © 2011-2015 Quentin "Sardem FF7" Glidic
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

#include "config.h"

#include <string.h>
#include <glib.h>
#include <weechat-plugin.h>
#include <libeventd-event.h>
#include <libeventc-light.h>

typedef struct {
    EventcLightConnection *client;
    gint fd;
    struct t_hook *fd_hook;
    struct t_hook *connect_hook;
    struct t_hook *print_hook;
    struct t_hook *command_hook;
    struct {
        struct t_config_file *file;
        struct {
            struct t_config_section *section;

            struct t_config_option *highlight;
            struct t_config_option *chat;
            struct t_config_option *im;
            struct t_config_option *notice;
            struct t_config_option *action;
            struct t_config_option *notify;
            struct t_config_option *join;
            struct t_config_option *leave;
            struct t_config_option *quit;
        } events;
        struct {
            struct t_config_section *section;

            struct t_config_option *ignore_current_buffer;
            struct t_config_option *blacklist;
        } restrictions;
    } config;
    GHashTable *blacklist;
} WecContext;

#define _wec_config_boolean(sname, name) weechat_config_boolean(_wec_context.config.sname.name)

struct t_weechat_plugin *weechat_plugin = NULL;
static WecContext _wec_context;

WEECHAT_PLUGIN_NAME("eventc");
WEECHAT_PLUGIN_DESCRIPTION("Client for eventd");
WEECHAT_PLUGIN_AUTHOR("Quentin Glidic <sardemff7+weechat@sardemff7.net>");
WEECHAT_PLUGIN_VERSION("0.1");
WEECHAT_PLUGIN_LICENSE("GPL3");


static gboolean _wec_connect(void);
static gint
_wec_fd_callback(gconstpointer user_data, gpointer data, gint fd)
{
    if ( eventc_light_connection_read(_wec_context.client) < 0 )
        _wec_connect();

    return WEECHAT_RC_OK;
}

static gint
_wec_try_connect(gconstpointer user_data, gpointer data, gint remaining_calls)
{
    gint error = 0;
    if ( eventc_light_connection_is_connected(_wec_context.client, &error) )
        return WEECHAT_RC_OK;
    if ( _wec_context.connect_hook != NULL )
        return WEECHAT_RC_ERROR;

    if ( eventc_light_connection_connect(_wec_context.client) < 0 )
    {
        _wec_context.connect_hook = weechat_hook_timer(GPOINTER_TO_UINT(user_data) * 1000, 60, 1, _wec_try_connect, GUINT_TO_POINTER(GPOINTER_TO_UINT(user_data) * 2), NULL);
        return WEECHAT_RC_ERROR;
    }

    _wec_context.connect_hook = NULL;
    _wec_context.fd = eventc_light_connection_get_socket(_wec_context.client);
    _wec_context.fd_hook = weechat_hook_fd(_wec_context.fd, 1, 0, 0, _wec_fd_callback, NULL, NULL);
    return WEECHAT_RC_OK;
}

static gboolean
_wec_connect(void)
{
    return ( _wec_try_connect(GUINT_TO_POINTER(2), NULL, 1) == WEECHAT_RC_OK );

}

static void
_wec_disconnect(void)
{
    if ( _wec_context.connect_hook != NULL )
        weechat_unhook(_wec_context.connect_hook);

    gint error = 0;
    if ( eventc_light_connection_is_connected(_wec_context.client, &error) )
    {
        weechat_unhook(_wec_context.fd_hook);
        eventc_light_connection_close(_wec_context.client);
    }

    _wec_context.connect_hook = NULL;
    _wec_context.fd = 0;
    _wec_context.fd_hook = NULL;
}

#define _wec_define_section(name) _wec_context.config.name.section = weechat_config_new_section(_wec_context.config.file, #name, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
#define _wec_define_boolean(sname, name, default_value, description) _wec_define_boolean_(sname, #name, name, default_value, description)
#define _wec_define_boolean_(sname, name, member, default_value, description) _wec_context.config.sname.member = weechat_config_new_option(_wec_context.config.file, _wec_context.config.sname.section, name, "boolean", description, NULL, 0, 0, default_value, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
#define _wec_define_string(sname, name, default_value, description) _wec_context.config.sname.name = weechat_config_new_option(_wec_context.config.file, _wec_context.config.sname.section, #name, "string", description, NULL, 0, 0, default_value, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

static void
_wec_config_blacklist_update(gpointer data, struct t_config_option *option)
{
    g_return_if_fail(option == _wec_context.config.restrictions.blacklist);

    g_hash_table_remove_all(_wec_context.blacklist);

    const gchar *nick, *s;
    nick = weechat_config_string(option);
    while ( ( s = g_utf8_strchr(nick, -1, ' ') ) != NULL )
    {
        g_hash_table_add(_wec_context.blacklist, g_strndup(nick, s - nick));
        nick = s + 1;
    }
    g_hash_table_add(_wec_context.blacklist, g_strdup(nick));
}

static void
_wec_config_init(void)
{
    _wec_context.config.file = weechat_config_new("eventc", NULL, NULL, NULL);

    _wec_define_section(events);
    _wec_define_boolean(events, highlight, "on", "Highlight (channel and private), they are treated as normal messages (and filtred as such) if this setting is 'off'");
    _wec_define_boolean(events, chat, "off", "Channel messages");
    _wec_define_boolean(events, im, "on", "Private messages");
    _wec_define_boolean(events, notice, "on", "Notices");
    _wec_define_boolean(events, action, "off", "Action messages (/me)");
    _wec_define_boolean(events, notify, "on", "Presence notifications");
    _wec_define_boolean(events, join, "off", "Channel join");
    _wec_define_boolean(events, leave, "off", "Channel leave");
    _wec_define_boolean(events, quit, "off", "Channel quit");

    _wec_define_section(restrictions);
    _wec_define_boolean_(restrictions, "ignore-current-buffer", ignore_current_buffer, "on", "Ignore messages from currently displayed buffer");
    _wec_define_string(restrictions, blacklist, "", "A (space-separated) list of nicknames to completely ignore");

    switch ( weechat_config_read(_wec_context.config.file) )
    {
    case WEECHAT_CONFIG_READ_OK:
    break;
    case WEECHAT_CONFIG_READ_MEMORY_ERROR:
    break;
    case WEECHAT_CONFIG_READ_FILE_NOT_FOUND:
        weechat_config_write(_wec_context.config.file);
    break;
    }
}

static gchar *
_wec_split_message(const gchar *s)
{
    gchar *c1, *c2;
    c1 = g_utf8_strchr(s, -1, '"');
    if ( c1 == NULL )
        return NULL;
    ++c1;

    c2 = g_utf8_strchr(c1, -1, '"');
    if ( c2 == NULL )
        return NULL;

    return g_strndup(c1, c2 - c1);
}

static gint
_wec_print_callback(gconstpointer user_data, gpointer data, struct t_gui_buffer *buffer, time_t date, gint tags_count, const gchar **tags, gint displayed, gint highlight, const gchar *prefix, const gchar *message)
{
    if ( ( ! displayed ) || ( buffer == NULL ) )
        return WEECHAT_RC_OK;

    const gchar *plugin = weechat_buffer_get_string(buffer, "plugin");
    if ( g_strcmp0(plugin, "irc") != 0 )
        return WEECHAT_RC_OK;

    if ( _wec_config_boolean(restrictions, ignore_current_buffer) && ( buffer == weechat_current_buffer() ) )
        return WEECHAT_RC_OK;

    gint error = 0;
    if ( ! eventc_light_connection_is_connected(_wec_context.client, &error) )
        return WEECHAT_RC_OK;

    const gchar *category = NULL;
    const gchar *name = NULL;

    const gchar *channel = NULL;

    const gchar *buffer_type = weechat_buffer_get_string(buffer, "localvar_type");
    if ( g_strcmp0(buffer_type, "channel") == 0 )
    {
        category = "chat";
        channel = weechat_buffer_get_string(buffer, "localvar_channel");
    }
    else if ( g_strcmp0(buffer_type, "private") == 0 )
        category = "im";
    else
        return WEECHAT_RC_OK;

    const gchar *nick = NULL;
    gchar *msg = NULL;

    gint i;
    for ( i = 0 ; i < tags_count  ; ++i )
    {
        const gchar *tag = tags[i];
        if ( g_str_has_prefix(tag, "log") || g_str_has_prefix(tag, "no_") )
            continue;

        if ( ( g_strcmp0(tag, "away_info") == 0 ) || ( g_strcmp0(tag, "notify_none") == 0 ) )
            goto cleanup;

        if ( g_str_has_prefix(tag, "irc_") )
        {
            tag += strlen("irc_");

            if ( g_strcmp0(tag, "privmsg") == 0 )
            {
                if ( highlight && _wec_config_boolean(events, highlight) )
                {
                    name = "highlight";
                    continue;
                }
                if ( ( channel != NULL ) && ( ! _wec_config_boolean(events, chat) ) )
                    break;

                if ( ! _wec_config_boolean(events, im) )
                    break;

                name = "received";
            }
            else if ( g_strcmp0(tag, "notice") == 0 )
            {
                category = "im";
                if ( highlight && _wec_config_boolean(events, highlight) )
                {
                    name = "highlight";
                    continue;
                }

                if ( ! _wec_config_boolean(events, notice) )
                    break;

                name = "received";
            }
            else if ( g_str_has_prefix(tag, "notify_") )
            {
                if ( ! _wec_config_boolean(events, notify) )
                    break;

                tag += strlen("notify_");

                category = "presence";
                if ( g_strcmp0(tag, "join") == 0 )
                    name = "signed-on";
                else if ( g_strcmp0(tag, "quit") == 0 )
                    name = "signed-off";
                else if ( g_strcmp0(tag, "back") == 0 )
                    name = "back";
                else if ( g_strcmp0(tag, "away") == 0 )
                {
                    name = "away";
                    msg = _wec_split_message(message);
                }
                else if ( g_strcmp0(tag, "still_away") == 0 )
                {
                    name = "message";
                    msg = _wec_split_message(message);
                }
            }
            else if ( g_strcmp0(tag, "join") == 0 )
            {
                if ( ! _wec_config_boolean(events, join) )
                    break;

                category = "presence";
                name = "join";
            }
            else if ( g_strcmp0(tag, "leave") == 0 )
            {
                if ( ! _wec_config_boolean(events, leave) )
                    break;

                category = "presence";
                name = "leave";
            }
            else if ( g_strcmp0(tag, "quit") == 0 )
            {
                if ( ! _wec_config_boolean(events, quit) )
                    break;

                category = "presence";
                name = "signed-off";
            }
        }
        else if ( g_str_has_prefix(tag, "nick_") )
            nick = tag + strlen("nick_");
    }

    if ( name == NULL )
        goto cleanup;

    if ( g_hash_table_contains(_wec_context.blacklist, nick) )
        goto cleanup;

    EventdEvent *event;

    event = eventd_event_new(category, name);

    if ( nick != NULL )
        eventd_event_add_data(event, g_strdup("buddy-name"), g_strdup(nick));

    if ( channel != NULL )
        eventd_event_add_data(event, g_strdup("channel"), g_strdup(channel));

    eventd_event_add_data(event, g_strdup("message"), ( msg != NULL ) ? msg : g_strdup(message));
    msg = NULL;

    eventc_light_connection_event(_wec_context.client, event);
    eventd_event_unref(event);

cleanup:
    g_free(msg);
    return WEECHAT_RC_OK;
}

gint
_wec_command(gconstpointer user_data, gpointer data, struct t_gui_buffer *buffer, gint argc, gchar **argv, gchar **argv_eol)
{
    if ( argc < 2 )
        return WEECHAT_RC_ERROR;

    if ( g_strcmp0(argv[1], "connect") == 0 )
    {
        _wec_disconnect();
        _wec_connect();
    }
    else if ( g_strcmp0(argv[1], "disconnect") == 0 )
        _wec_disconnect();
    else
        return WEECHAT_RC_ERROR;

    return WEECHAT_RC_OK;
}


int
weechat_plugin_init(struct t_weechat_plugin *plugin, gint argc, gchar *argv[])
{
    weechat_plugin = plugin;

    _wec_context.blacklist = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    _wec_config_init();

    _wec_context.client = eventc_light_connection_new(NULL);
    _wec_connect();

    _wec_context.print_hook = weechat_hook_print(NULL, NULL, NULL, 1, _wec_print_callback, NULL, NULL);
    _wec_context.command_hook = weechat_hook_command("eventc", "Control eventc", "connect | disconnect", "", "connect || disconnect", _wec_command, NULL, NULL);

    return WEECHAT_RC_OK;
}

int
weechat_plugin_end(struct t_weechat_plugin *plugin)
{
    _wec_disconnect();

    eventc_light_connection_unref(_wec_context.client);

    g_hash_table_unref(_wec_context.blacklist);

    return WEECHAT_RC_OK;
}
