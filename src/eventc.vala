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
        namespace Callback
        {
            private static int
            print(Weechat.Buffer? buffer, time_t date, string*[] tags, bool displayed, bool highlight, string? prefix, string message)
            {
                if ( ! eventc.is_connected() )
                    return Weechat.Rc.ERROR;

                if ( ( ! displayed ) || ( buffer == null ) || ( buffer.get_string("plugin") != "irc" ) || ( buffer == Weechat.current_buffer() ) )
                    return Weechat.Rc.OK;

                Eventd.Event event = null;
                unowned string channel = null;
                string nick = null;
                string msg = null;

                unowned string name = null;
                switch ( buffer.get_string("localvar_type") )
                {
                case "channel":
                    channel = buffer.get_string("localvar_channel");
                break;
                case "private":
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
                                name = "highlight-msg";
                                continue;
                            }
                            if ( channel != null )
                            {
                                if ( Config.Events.no_chat_msg() )
                                    return Weechat.Rc.OK;
                                name = "chat-msg";
                            }
                            else
                            {
                                if ( Config.Events.no_im_msg() )
                                    return Weechat.Rc.OK;
                                name = "im-msg";
                            }
                        break;
                        case "notice":
                            if ( highlight && Config.Events.do_highlight() )
                            {
                                name = "highligh-notice";
                                continue;
                            }
                            if ( Config.Events.no_notice() )
                                return Weechat.Rc.OK;
                            name = "notice";
                        break;
                        case "action":
                            if ( highlight && Config.Events.do_highlight() )
                            {
                                name = "highligh-action";
                                continue;
                            }
                            if ( Config.Events.no_action() )
                                return Weechat.Rc.OK;
                            name = "action";
                        break;
                        case "notify":
                            if ( Config.Events.no_notify() )
                                return Weechat.Rc.OK;
                            if ( stag.length < 3 )
                                break;
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
                                name = "change-status-message";
                                msg = message.split("\"")[1];
                            break;
                            }
                        break;
                        case "join":
                            if ( Config.Events.no_join() )
                                return Weechat.Rc.OK;
                            name = "join";
                        break;
                        case "leave":
                            if ( Config.Events.no_leave() )
                                return Weechat.Rc.OK;
                            name = "leave";
                        break;
                        case "quit":
                            if ( Config.Events.no_quit() )
                                return Weechat.Rc.OK;
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
                        }
                    break;
                    }
                }
                if ( name == null )
                    return Weechat.Rc.OK;
                if ( name.has_prefix("highlight") )
                {
                    if ( Config.Events.in_highlight_blacklist(nick) )
                        return Weechat.Rc.OK;
                }
                else if ( Config.Events.in_blacklist(nick) )
                    return Weechat.Rc.OK;

                event = new Eventd.Event(name);
                if ( nick != null )
                    event.add_data("nick", (owned)nick);
                if ( channel != null )
                    event.add_data("channel", channel);
                event.add_data("message", ( msg != null ) ? msg : message);

                eventc.event.begin(event, (obj, res) => {
                    try
                    {
                        eventc.event.end(res);
                    }
                    catch ( Eventc.EventcError e )
                    {
                        Weechat.printf(null, "eventc: Error sending event: %s", e.message);
                        connect();
                    }
                });
                return  Weechat.Rc.OK;
            }

            private static int
            server_info_changed(string name, string @value)
            {
                if ( ! eventc.is_connected() )
                    return Weechat.Rc.ERROR;

                eventc.host = Config.get_host();
                eventc.port = Config.get_port();
                connect();
                return  Weechat.Rc.OK;
            }

            private static int
            timeout_changed(string name, string @value)
            {
                eventc.timeout = Config.get_timeout();
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
                    if ( eventc.is_connected() )
                        disconnect(() => connect);
                    else
                        connect();
                break;
                case "disconnect":
                    disconnect();
                break;
                case "event":
                    if ( ! eventc.is_connected() )
                        return Weechat.Rc.ERROR;
                    var event = new Eventd.Event(args[2]);
                    eventc.event(event);
                break;
                }
                return  Weechat.Rc.OK;
            }
        }

        private static Eventc.Connection eventc;
        private static unowned Weechat.Hook connect_hook = null;
        private static uint64 tries;

        private static int
        connect(int remaining_calls = 0)
        {
            eventc.connect.begin((obj, res) => {
                try
                {
                    eventc.connect.end(res);
                }
                catch ( Eventc.EventcError e )
                {
                    Weechat.printf(null, "eventc: Error connecting to eventd: %s", e.message);
                    var max_tries = Config.get_max_tries();
                    if ( ( max_tries == 0 ) || ( ++tries < max_tries ) )
                        connect_hook = Weechat.hook_timer(Config.get_retry_delay() * 1000, 0, 1, connect);
                    return;
                }
                tries = 0;
                Weechat.printf(null, "eventc: Connected to eventd");
            });

            return Weechat.Rc.OK;
        }

        public static void
        disconnect(GLib.Callback? @callback = null)
        {
            Weechat.unhook(connect_hook);

            if ( ! eventc.is_connected() )
            {
                Weechat.printf(null, "eventc: Not connected");
                if ( @callback != null )
                    @callback();
                return;
            }

            eventc.close.begin((obj, res) => {
                try
                {
                    eventc.close.end(res);
                }
                catch ( Eventc.EventcError e )
                {
                    Weechat.printf(null, "eventc: Error while closing connection to eventd: %s", e.message);
                }
                if ( @callback != null )
                    @callback();
            });
        }

        public static void
        init(string[] args)
        {
            Config.init();
            Config.read();

            eventc = new Eventc.Connection(Config.get_host(), Config.get_port(), Config.get_category());

            eventc.timeout = Config.get_timeout();
            eventc.enable_proxy = false;

            connect();

            Weechat.hook_command("eventc", "Control eventc",
                                 "connect | disconnect", "",
                                 "connect || disconnect || event",
                                 Callback.command);
            Weechat.hook_print(null, null, null, true, Callback.print);
            Weechat.hook_config("eventc.server.*", Callback.server_info_changed);
            Weechat.hook_config("eventc.connection.timeout", Callback.timeout_changed);
        }

        public static void
        end()
        {
            Config.clean();
            eventc = null;
        }
    }
}
