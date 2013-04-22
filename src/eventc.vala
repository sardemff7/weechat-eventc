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

namespace Eventd
{
    namespace WeechatPlugin
    {
        Weechat.Buffer debug_buffer = null;

        static void
        log_handler(string? domain, GLib.LogLevelFlags level, string message)
        {
            if ( debug_buffer == null )
                return;

            unowned string log_level_message = "";

            switch ( level & GLib.LogLevelFlags.LEVEL_MASK )
            {
                case GLib.LogLevelFlags.LEVEL_ERROR:
                    log_level_message = "ERROR";
                break;
                case GLib.LogLevelFlags.LEVEL_CRITICAL:
                    log_level_message = "CRITICAL";
                break;
                case GLib.LogLevelFlags.LEVEL_WARNING:
                    log_level_message = "WARNING";
                break;
                case GLib.LogLevelFlags.LEVEL_MESSAGE:
                    log_level_message = "MESSAGE";
                break;
                case GLib.LogLevelFlags.LEVEL_INFO:
                    log_level_message = "INFO";
                break;
                case GLib.LogLevelFlags.LEVEL_DEBUG:
                    log_level_message = "DEBUG";
                break;
            }

            GLib.StringBuilder full_message;

            full_message = new GLib.StringBuilder("(");

            full_message.append(log_level_message);

            if ( domain != null )
                full_message.append_printf(" [%s]", domain);

            full_message.append("\t");
            full_message.append(message);

            Weechat.printf(debug_buffer, full_message.str);
        }

        static int
        debug_buffer_close()
        {
            debug_buffer = null;
            return Weechat.Rc.OK;
        }


        static bool
        is_disconnected()
        {
            bool r = true;
            try
            {
                r = ( ! eventc.is_connected() );
            }
            catch ( Eventc.Error e )
            {
                Weechat.printf(null, "eventc: Error checking connection event: %s", e.message);
                connect();
            }
            return r;
        }
        namespace Callback
        {
            private static async void
            send(Eventd.Event event)
            {
                try
                {
                    eventc.event(event);
                }
                catch ( Eventc.Error e )
                {
                    Weechat.printf(null, "eventc: Error sending event: %s", e.message);
                    connect();
                }
            }

            private static int
            print(Weechat.Buffer? buffer, time_t date, string*[] tags, bool displayed, bool highlight, string? prefix, string message)
            {
                if ( ( ! displayed ) || ( buffer == null ) || ( buffer.get_string("plugin") != "irc" ) || ( buffer == Weechat.current_buffer() ) )
                    return Weechat.Rc.OK;

                if ( is_disconnected() )
                    return Weechat.Rc.ERROR;


                Eventd.Event event = null;
                unowned string channel = null;
                string nick = null;
                string msg = null;

                unowned string category = null;
                unowned string name = null;
                switch ( buffer.get_string("localvar_type") )
                {
                case "channel":
                    category = "chat";
                    channel = buffer.get_string("localvar_channel");
                break;
                case "private":
                    category = "im";
                break;
                }

                foreach ( string tag in tags )
                {
                    if ( tag.has_prefix("log") )
                        continue;
                    if ( tag.has_prefix("no_") )
                        continue;
                    if ( tag == "away_info" )
                        return Weechat.Rc.OK;

                    var stag = tag.split("_", 3);
                    switch ( stag[0] )
                    {
                    case "irc":
                        switch ( stag[1] )
                        {
                        case "privmsg":
                            if ( highlight && Config.Events.do_highlight() )
                            {
                                name = "highlight";
                                continue;
                            }
                            if ( channel != null )
                            {
                                if ( Config.Events.no_chat_msg() )
                                    return Weechat.Rc.OK;
                            }
                            else
                            {
                                if ( Config.Events.no_im_msg() )
                                    return Weechat.Rc.OK;
                            }
                            name = "received";
                        break;
                        case "notice":
                        category = "im";
                            if ( highlight && Config.Events.do_highlight() )
                            {
                                name = "highlight";
                                continue;
                            }
                            if ( Config.Events.no_notice() )
                                return Weechat.Rc.OK;
                            name = "received";
                        break;
                        case "notify":
                            if ( Config.Events.no_notify() )
                                return Weechat.Rc.OK;
                            if ( stag.length < 3 )
                                break;
                            category = "presence";
                            switch ( stag[2] )
                            {
                            case "back":
                                name = "back";
                            break;
                            case "away":
                                name = "away";
                                msg = message.split("\"")[1];
                            break;
                            case "still_away":
                                name = "message";
                                msg = message.split("\"")[1];
                            break;
                            }
                        break;
                        case "join":
                            if ( Config.Events.no_join() )
                                return Weechat.Rc.OK;
                            category = "presence";
                            name = "join";
                        break;
                        case "leave":
                            if ( Config.Events.no_leave() )
                                return Weechat.Rc.OK;
                            category = "presence";
                            name = "leave";
                        break;
                        case "quit":
                            if ( Config.Events.no_quit() )
                                return Weechat.Rc.OK;
                            category = "presence";
                            name = "quit";
                        break;
                        }
                    break;
                    case "nick":
                        nick = string.joinv("_", stag[1:-1]);
                    break;
                    case "notify":
                        switch ( stag[1] )
                        {
                        case "none":
                            return Weechat.Rc.OK;
                        case "join":
                            category = "presence";
                            name = "online";
                        break;
                        case "quit":
                            category = "presence";
                            name = "offline";
                        break;
                        }
                    break;
                    }
                }
                if ( name == null )
                    return Weechat.Rc.OK;
                if ( highlight )
                {
                    if ( Config.Events.in_highlight_blacklist(nick) )
                        return Weechat.Rc.OK;
                }
                else if ( Config.Events.in_blacklist(nick) )
                    return Weechat.Rc.OK;

                event = new Eventd.Event(category, name);
                if ( nick != null )
                    event.add_data("buddy-name", (owned)nick);
                if ( channel != null )
                    event.add_data("channel", channel);
                event.add_data("message", ( msg != null ) ? msg : message);

                send.begin(event);

                return  Weechat.Rc.OK;
            }

            private static int
            server_info_changed(string name, string @value)
            {
                if ( is_disconnected() )
                    return Weechat.Rc.ERROR;

                eventc.host = Config.get_host();
                connect();
                return  Weechat.Rc.OK;
            }

            private static int
            command(Weechat.Buffer? buffer, string[] args, string[] args_eol)
            {
                if ( args.length < 2 )
                    return Weechat.Rc.ERROR;

                switch ( args[1] )
                {
                case "connect":
                    if ( is_disconnected() )
                        connect();
                    else
                    {
                        disconnect();
                        connect();
                    }
                break;
                case "disconnect":
                    disconnect();
                break;
                case "debug":
                    debug_buffer = new Weechat.Buffer("eventc-debug", null, debug_buffer_close);
                break;
                }
                return  Weechat.Rc.OK;
            }
        }

        private static Eventc.Connection eventc;
        private static unowned Weechat.Hook connect_hook = null;
        private static uint64 tries;

        private static int
        connect(int remaining = -1)
        {
            try
            {
                eventc.connect_sync();
            }
            catch ( Eventc.Error e )
            {
                Weechat.printf(null, "eventc: Error connecting to eventd: %s", e.message);
                var max_tries = Config.get_max_tries();
                if ( ( max_tries == 0 ) || ( ++tries < max_tries ) )
                {
                    connect_hook = Weechat.hook_timer(Config.get_retry_delay() * 1000, 0, 1, connect);
                    return Weechat.Rc.OK;
                }
                return Weechat.Rc.ERROR;
            }
            tries = 0;
            Weechat.printf(null, "eventc: Connected to eventd");
            return  Weechat.Rc.OK;
        }

        private static void
        disconnect()
        {
            Weechat.unhook(connect_hook);

            if ( is_disconnected() )
            {
                Weechat.printf(null, "eventc: Not connected");
                return;
            }

            try
            {
                eventc.close();
            }
            catch ( Eventc.Error e )
            {
                Weechat.printf(null, "eventc: Error while closing connection to eventd: %s", e.message);
            }
        }

        public static void
        init(string[] args)
        {
            Config.init();
            Config.read();

            eventc = new Eventc.Connection(Config.get_host());

            eventc.passive = true;
            eventc.enable_proxy = false;

            connect();

            GLib.Log.set_default_handler(log_handler);

            Weechat.hook_command("eventc", "Control eventc",
                                 "connect | disconnect | debug", "",
                                 "connect || disconnect | debug",
                                 Callback.command);
            Weechat.hook_print(null, null, null, true, Callback.print);
            Weechat.hook_config("eventc.server.*", Callback.server_info_changed);
        }

        public static void
        end()
        {
            disconnect();
            Config.clean();
            eventc = null;
        }
    }
}
