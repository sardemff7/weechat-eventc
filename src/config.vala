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


namespace Eventd.WeechatPlugin.Config
{
    private static Weechat.Config file;

    private static Weechat.Config.Section server;
    private static Weechat.Config.Section client;
    private static Weechat.Config.Section connection;

    private static Weechat.Config.Option host;
    private static Weechat.Config.Option port;

    private static Weechat.Config.Option category;

    private static Weechat.Config.Option max_tries;
    private static Weechat.Config.Option retry_delay;
    private static Weechat.Config.Option timeout;

    public unowned string get_host() { return host.@string(); }
    public uint16 get_port() { return (uint16)port.integer(); }

    public unowned string get_category() { return category.@string(); }

    public int get_max_tries() { return max_tries.integer(); }
    public uint16 get_retry_delay() { return (uint16)retry_delay.integer(); }
    public uint get_timeout() { return (uint)timeout.integer(); }

    public static void
    init()
    {
        file = new Weechat.Config("eventc");

        server = new Weechat.Config.Section(file, "server", false, false);
        client = new Weechat.Config.Section(file, "client", false, false);
        connection = new Weechat.Config.Section(file, "connection", false, false);

        host = new Weechat.Config.Option(file, server, "host", "string", "", null, 0, 0, "localhost", null, false);
        port = new Weechat.Config.Option(file, server, "port", "integer", "", null, 0, 65535, "7100", null, false);

        category = new Weechat.Config.Option(file, client, "category", "string", "", null, 0, 0, "irc", null, false);

        max_tries = new Weechat.Config.Option(file, connection, "max-tries", "integer", "", null, -1, 100, "3", null, false);
        retry_delay = new Weechat.Config.Option(file, connection, "retry-delay", "integer", "", null, 0, 3600, "10", null, false);
        timeout = new Weechat.Config.Option(file, connection, "timeout", "integer", "", null, 0, 3600, "3", null, false);
    }

    public static void
    read()
    {
        file.read();
    }

    public static void
    clean()
    {
        file = null;
    }
}
