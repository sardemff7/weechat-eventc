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
    struct t_config_option *option;
    gboolean whitelist;
    GHashTable *names;
} WecEventFilter;

typedef struct {
    struct t_gui_buffer *buffer;
    EventcLightConnection *client;
    gboolean want_connected;
    gint reconnect_count;
    gint fd;
    struct t_hook *fd_hook;
    struct t_hook *connect_hook;
    struct t_hook *print_hook;
    struct t_hook *buffer_closing_hook;
    struct t_hook *command_hook;
    struct {
        struct t_config_file *file;
        struct {
            struct t_config_section *section;

            WecEventFilter highlight;
            WecEventFilter chat;
            WecEventFilter im;
            WecEventFilter notice;
            WecEventFilter action;
            WecEventFilter notify;
            WecEventFilter join;
            WecEventFilter leave;
            WecEventFilter quit;
        } events;
        struct {
            struct t_config_section *section;

            struct t_config_option *ignore_current_buffer;
            WecEventFilter nick_filter;
        } restrictions;
    } config;
} WecContext;

#define _wec_config_boolean(sname, name) weechat_config_boolean(context->config.sname.name)

struct t_weechat_plugin *weechat_plugin = NULL;
static WecContext _wec_context;

WEECHAT_PLUGIN_NAME("eventc");
WEECHAT_PLUGIN_DESCRIPTION("Client for eventd");
WEECHAT_PLUGIN_AUTHOR("Quentin Glidic <sardemff7+weechat@sardemff7.net>");
WEECHAT_PLUGIN_VERSION("0.1");
WEECHAT_PLUGIN_LICENSE("GPL3");


static gboolean _wec_connect(WecContext *context);
static gint
_wec_fd_callback(gconstpointer user_data, gpointer data, gint fd)
{
    WecContext *context = (WecContext *) user_data;

    eventc_light_connection_read(context->client);

    return WEECHAT_RC_OK;
}

static gint
_wec_try_connect(gconstpointer user_data, gpointer data, gint remaining_calls)
{
    WecContext *context = (WecContext *) user_data;

    gint error = 0;
    if ( eventc_light_connection_is_connected(context->client, &error) )
        return WEECHAT_RC_OK;

    if ( eventc_light_connection_connect(context->client) < 0 )
    {
        context->connect_hook = weechat_hook_timer((1000 << ++context->reconnect_count), 60, 1, _wec_try_connect, context, NULL);
        return WEECHAT_RC_ERROR;
    }

    g_debug("Connected");

    context->reconnect_count = 0;
    context->connect_hook = NULL;
    context->fd = eventc_light_connection_get_socket(context->client);
    context->fd_hook = weechat_hook_fd(context->fd, 1, 0, 0, _wec_fd_callback, context, NULL);
    return WEECHAT_RC_OK;
}

static gboolean
_wec_connect(WecContext *context)
{
    context->want_connected = TRUE;

    return ( _wec_try_connect(context, NULL, 0) == WEECHAT_RC_OK );
}

static void
_wec_disconnected_callback(EventcLightConnection *client, gpointer user_data)
{
    WecContext *context = user_data;

    g_debug("Disconnected");
    if ( context->want_connected )
        _wec_connect(context);
}

static void
_wec_disconnect(WecContext *context)
{
    context->want_connected = FALSE;

    if ( context->connect_hook != NULL )
        weechat_unhook(context->connect_hook);

    gint error = 0;
    if ( eventc_light_connection_is_connected(context->client, &error) )
    {
        weechat_unhook(context->fd_hook);
        eventc_light_connection_close(context->client);
    }

    context->reconnect_count = 0;
    context->connect_hook = NULL;
    context->fd = 0;
    context->fd_hook = NULL;
}

#define _wec_define_section(name) G_STMT_START { \
        context->config.name.section = weechat_config_new_section(context->config.file, #name, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL); \
    } G_STMT_END
#define _wec_define_boolean(sname, name, member, default_value, description) G_STMT_START { \
        context->config.sname.member = weechat_config_new_option(context->config.file, context->config.sname.section, name, "boolean", description, NULL, 0, 0, default_value, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL); \
    } G_STMT_END
#define _wec_define_string(sname, name, member, default_value, description) G_STMT_START { \
        context->config.sname.member = weechat_config_new_option(context->config.file, context->config.sname.section, #name, "string", description, NULL, 0, 0, default_value, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL); \
    } G_STMT_END
#define _wec_define_filter(sname, name, member, default_value, description) G_STMT_START { \
        context->config.sname.member.option = weechat_config_new_option(context->config.file, context->config.sname.section, name, "string", description, NULL, 0, 0, default_value, NULL, 0, _wec_filter_check, &context->config.sname.member, NULL, _wec_filter_update, &context->config.sname.member, NULL, NULL, NULL, NULL); \
        context->config.sname.member.names = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL); \
    } G_STMT_END
#define _wec_define_filter_simple(sname, name, default_value, description) _wec_define_filter(sname, #name, name, default_value, description)
#define _wec_process_filter(sname, member) G_STMT_START { \
        _wec_filter_update(&context->config.sname.member, NULL, context->config.sname.member.option); \
    } G_STMT_END
#define _wec_clean_filter(sname, member) G_STMT_START { \
        g_hash_table_unref(context->config.sname.member.names); \
    } G_STMT_END

#define _wec_event_ignore(name) _wec_filter_ignore(&context->config.events.name, buffer_name)
static inline gboolean
_wec_filter_ignore(WecEventFilter *filter, const gchar *name)
{
    if ( name == NULL )
        return TRUE;
    return ( filter->whitelist != g_hash_table_contains(filter->names, name) );
}

static gint
_wec_filter_check(gconstpointer user_data, gpointer data, struct t_config_option *option, const gchar *value)
{
    WecEventFilter *filter = (WecEventFilter *) user_data;

    g_return_val_if_fail(filter->option == option, WEECHAT_RC_ERROR);

    if ( g_utf8_get_char(value) != '+' )
        return 1;

    value = g_utf8_next_char(value);
    gboolean want_end = TRUE;
    if ( g_utf8_get_char(value) == ' ' )
    {
        want_end = FALSE;
        value = g_utf8_next_char(value);
    }
    if ( g_utf8_get_char(value) == '\0' )
        return want_end ? 1 : 0;
    return 1;
}

static void
_wec_filter_update(gconstpointer user_data, gpointer data, struct t_config_option *option)
{
    WecEventFilter *filter = (WecEventFilter *) user_data;

    g_return_if_fail(filter->option == option);

    const gchar *value = weechat_config_string(filter->option);

    g_hash_table_remove_all(filter->names);

    filter->whitelist = ( g_utf8_get_char(value) == '+' );
    if ( filter->whitelist )
    {
        value = g_utf8_next_char(value);
        if ( g_utf8_get_char(value) == '\0' )
            return;
        value = g_utf8_next_char(value);
    }

    gchar **list, **n;
    list = g_strsplit(value, " ", -1);
    for ( n = list ; *n != NULL ; ++n )
    {
        if ( g_utf8_get_char(*n) == '\0' )
            g_free(*n);
        else
            g_hash_table_add(filter->names, *n);
    }
    g_free(list);
}

static void
_wec_config_init(WecContext *context)
{
    context->config.file = weechat_config_new("eventc", NULL, NULL, NULL);

    _wec_define_section(events);
    _wec_define_filter_simple(events, highlight, "", "Highlight");
    _wec_define_filter_simple(events, chat, "+", "Channel messages");
    _wec_define_filter_simple(events, im, "", "Private messages");
    _wec_define_filter_simple(events, notice, "", "Notices");
    _wec_define_filter_simple(events, action, "+", "Action messages (/me)");
    _wec_define_filter_simple(events, notify, "*", "Presence notifications");
    _wec_define_filter_simple(events, join, "+", "Channel join");
    _wec_define_filter_simple(events, leave, "+", "Channel leave");
    _wec_define_filter_simple(events, quit, "+", "Channel quit");

    _wec_define_section(restrictions);
    _wec_define_boolean(restrictions, "ignore-current-buffer", ignore_current_buffer, "on", "Ignore messages from currently displayed buffer");
    _wec_define_filter(restrictions, "nick-filter", nick_filter, "", "A (space-separated) list of nicknames to completely ignore");

    switch ( weechat_config_read(context->config.file) )
    {
    case WEECHAT_CONFIG_READ_OK:
    break;
    case WEECHAT_CONFIG_READ_MEMORY_ERROR:
    break;
    case WEECHAT_CONFIG_READ_FILE_NOT_FOUND:
        weechat_config_write(context->config.file);
    break;
    }

    _wec_process_filter(events, highlight);
    _wec_process_filter(events, chat);
    _wec_process_filter(events, im);
    _wec_process_filter(events, notice);
    _wec_process_filter(events, action);
    _wec_process_filter(events, notify);
    _wec_process_filter(events, join);
    _wec_process_filter(events, leave);
    _wec_process_filter(events, quit);
    _wec_process_filter(restrictions, nick_filter);
}

static void
_wec_config_uninit(WecContext *context)
{
    _wec_clean_filter(events, highlight);
    _wec_clean_filter(events, chat);
    _wec_clean_filter(events, im);
    _wec_clean_filter(events, notice);
    _wec_clean_filter(events, action);
    _wec_clean_filter(events, notify);
    _wec_clean_filter(events, join);
    _wec_clean_filter(events, leave);
    _wec_clean_filter(events, quit);
    _wec_clean_filter(restrictions, nick_filter);
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
    WecContext *context = (WecContext *) user_data;

    if ( ( ! displayed ) || ( buffer == NULL ) )
        return WEECHAT_RC_OK;

    const gchar *plugin = weechat_buffer_get_string(buffer, "plugin");
    if ( g_strcmp0(plugin, "irc") != 0 )
        return WEECHAT_RC_OK;

    if ( _wec_config_boolean(restrictions, ignore_current_buffer) && ( buffer == weechat_current_buffer() ) )
        return WEECHAT_RC_OK;

    gint error = 0;
    if ( ! eventc_light_connection_is_connected(context->client, &error) )
        return WEECHAT_RC_OK;

    const gchar *category = NULL;
    const gchar *name = NULL;

    const gchar *channel = NULL;
    const gchar *buffer_name = NULL;

    const gchar *buffer_type = weechat_buffer_get_string(buffer, "localvar_type");
    if ( g_strcmp0(buffer_type, "channel") == 0 )
    {
        category = "chat";
        channel = weechat_buffer_get_string(buffer, "localvar_channel");
        buffer_name = channel;
    }
    else if ( g_strcmp0(buffer_type, "private") == 0 )
    {
        category = "im";
        buffer_name = weechat_buffer_get_string(buffer, "localvar_channel");
    }

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
                if ( highlight )
                {
                    if ( _wec_event_ignore(highlight) )
                        break;
                    name = "highlight";
                    continue;
                }
                else if ( channel != NULL )
                {
                    if ( _wec_event_ignore(chat) )
                        break;
                }
                else if ( _wec_event_ignore(im) )
                    break;

                name = "received";
            }
            else if ( g_strcmp0(tag, "notice") == 0 )
            {
                category = "im";
                if ( highlight )
                {
                    if ( _wec_event_ignore(highlight) )
                        break;
                    name = "highlight";
                    continue;
                }
                else if ( _wec_event_ignore(notice) )
                    break;

                name = "received";
            }
            else if ( g_str_has_prefix(tag, "notify_") )
            {
                if ( _wec_event_ignore(notify) )
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
                if ( _wec_event_ignore(join) )
                    break;

                category = "presence";
                name = "join";
            }
            else if ( g_strcmp0(tag, "leave") == 0 )
            {
                if ( _wec_event_ignore(leave) )
                    break;

                category = "presence";
                name = "leave";
            }
            else if ( g_strcmp0(tag, "quit") == 0 )
            {
                if ( _wec_event_ignore(quit) )
                    break;

                category = "presence";
                name = "signed-off";
            }
        }
        else if ( g_str_has_prefix(tag, "nick_") )
            nick = tag + strlen("nick_");
    }

    if ( ( category == NULL ) || ( name == NULL ) )
        goto cleanup;

    if ( _wec_filter_ignore(&context->config.restrictions.nick_filter, nick) )
        goto cleanup;

    EventdEvent *event;

    event = eventd_event_new(category, name);

    if ( nick != NULL )
        eventd_event_add_data_string(event, g_strdup("buddy-name"), g_strdup(nick));

    if ( channel != NULL )
        eventd_event_add_data_string(event, g_strdup("channel"), g_strdup(channel));

    eventd_event_add_data_string(event, g_strdup("message"), ( msg != NULL ) ? msg : g_strdup(message));
    msg = NULL;

    eventc_light_connection_send_event(context->client, event);
    eventd_event_unref(event);

cleanup:
    g_free(msg);
    return WEECHAT_RC_OK;
}

gint
_wec_buffer_closing_callback(gconstpointer user_data, gpointer data, const gchar *signal, const gchar *type_data, gpointer signal_data)
{
    WecContext *context = (WecContext *) user_data;

    if ( context->buffer == signal_data )
        context->buffer = NULL;

    return WEECHAT_RC_OK;
}

gint
_wec_command(gconstpointer user_data, gpointer data, struct t_gui_buffer *buffer, gint argc, gchar **argv, gchar **argv_eol)
{
    WecContext *context = (WecContext *) user_data;

    if ( argc < 2 )
        return WEECHAT_RC_ERROR;

    if ( g_strcmp0(argv[1], "connect") == 0 )
    {
        _wec_disconnect(context);
        _wec_connect(context);
    }
    else if ( g_strcmp0(argv[1], "disconnect") == 0 )
        _wec_disconnect(context);
    else if ( g_strcmp0(argv[1], "debug") == 0 )
    {
        if ( context->buffer == NULL )
        {
            context->buffer = weechat_buffer_new("eventc", NULL, NULL, NULL, NULL, NULL, NULL);
            g_debug("Created debug buffer");
        }
        else
        {
            weechat_buffer_close(context->buffer);
            context->buffer = NULL;
        }
    }
    else
        return WEECHAT_RC_ERROR;

    return WEECHAT_RC_OK;
}

static void
_wec_log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
    WecContext *context = (WecContext *) user_data;

    struct t_gui_buffer *buffer = NULL;

    const gchar *prefix = weechat_prefix("action");
    switch ( log_level & G_LOG_LEVEL_MASK )
    {
        case G_LOG_LEVEL_ERROR:
        case G_LOG_LEVEL_CRITICAL:
        case G_LOG_LEVEL_WARNING:
            prefix = weechat_prefix("error");
        break;
        case G_LOG_LEVEL_MESSAGE:
        case G_LOG_LEVEL_INFO:
        case G_LOG_LEVEL_DEBUG:
            if ( context->buffer == NULL )
                return;
            buffer = context->buffer;
        break;
}
    weechat_printf(buffer, "%s[%s] %s", prefix, log_domain, message);

}

#if GLIB_CHECK_VERSION(2, 50, 0)
static GLogWriterOutput
_wec_log_writer(GLogLevelFlags log_level, const GLogField *fields, gsize n_fields, gpointer user_data)
{
    WecContext *context = (WecContext *) user_data;

    struct t_gui_buffer *buffer = NULL;
    const gchar *prefix = weechat_prefix("action");
    switch ( log_level & G_LOG_LEVEL_MASK )
    {
        case G_LOG_LEVEL_ERROR:
        case G_LOG_LEVEL_CRITICAL:
        case G_LOG_LEVEL_WARNING:
            prefix = weechat_prefix("error");
        break;
        case G_LOG_LEVEL_MESSAGE:
        case G_LOG_LEVEL_INFO:
        case G_LOG_LEVEL_DEBUG:
            if ( context->buffer == NULL )
                return G_LOG_WRITER_HANDLED;
            buffer = context->buffer;
        break;
    }

    const gchar *log_domain = NULL;
    const gchar *message = NULL;
    gsize i;
    for ( i = 0 ; i < n_fields ; ++i )
    {
        if ( g_strcmp0(fields[i].key, "MESSAGE") == 0 )
            message = fields[i].value;
        else if ( g_strcmp0(fields[i].key, "GLIB_DOMAIN") == 0 )
            log_domain = fields[i].value;
    }
    g_return_val_if_fail(message != NULL, G_LOG_WRITER_UNHANDLED);

    weechat_printf(buffer, "%s[%s] %s", prefix, log_domain, message);
    return G_LOG_WRITER_HANDLED;
}
#endif /* GLIB_CHECK_VERSION(2, 50, 0) */

int
weechat_plugin_init(struct t_weechat_plugin *plugin, gint argc, gchar *argv[])
{
    weechat_plugin = plugin;
    WecContext *context = &_wec_context;

    g_log_set_default_handler(_wec_log_handler, context);
#if GLIB_CHECK_VERSION(2, 50, 0)
    g_log_set_writer_func(_wec_log_writer, context, NULL);
#endif /* GLIB_CHECK_VERSION(2, 50, 0) */

    _wec_config_init(context);

    context->client = eventc_light_connection_new(NULL);

    eventc_light_connection_set_disconnected_callback(_wec_context.client, _wec_disconnected_callback, context, NULL);

    _wec_connect(context);

    context->print_hook = weechat_hook_print(NULL, NULL, NULL, 1, _wec_print_callback, context, NULL);
    context->buffer_closing_hook = weechat_hook_signal("buffer_closing", _wec_buffer_closing_callback, context, NULL);
    context->command_hook = weechat_hook_command("eventc", "Control eventc", "connect | disconnect | debug", "", "connect || disconnect || debug", _wec_command, context, NULL);

    return WEECHAT_RC_OK;
}

int
weechat_plugin_end(struct t_weechat_plugin *plugin)
{
    WecContext *context = &_wec_context;

    _wec_disconnect(context);

    eventc_light_connection_unref(context->client);

    _wec_config_uninit(context);

    return WEECHAT_RC_OK;
}
