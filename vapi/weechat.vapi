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

[CCode (cheader_filename = "weechat-plugin.h")]
namespace Weechat
{
    [CCode (instance_pos = 0.9)]
    public delegate int PrintCallback(Buffer? buffer, time_t date, [CCode (array_length_pos = 2.9)] string*[] tags, bool displayed, bool highlight, string prefix, string message);
    [CCode (instance_pos = 0.9)]
    public delegate int CommandCallback(Buffer? buffer, [CCode (array_length_cname = "args_lenght", array_length_pos = 1.9)] string[] args, [CCode (array_length_cname = "args_lenght", array_length_pos = 1.9)] string[] args_eol);
    [CCode (instance_pos = 0.9)]
    public delegate int TimerCallback(int remaining_calls);
    [CCode (instance_pos = 0.9)]
    public delegate int SignalCallback(string signal_name, string type_data, void *signal_data);
    [CCode (instance_pos = 0.9)]
    public delegate int ConfigCallback(string name, string @value);

    [PrintfFormat]
    public static void printf(Buffer? buffer, string format, ...);

    public static unowned Hook hook_print(Buffer? buffer, string? tags, string? message, bool strip_colors, [CCode (delegate_target_pos = 5.9)] PrintCallback callback_func);
    public static unowned Hook hook_command(string command, string description, string args, string args_description, string completion, [CCode (delegate_target_pos = 6.9)] CommandCallback callback_func);
    public static unowned Hook hook_timer(long interval, int align_second, int max_calls, [CCode (delegate_target_pos = 4.9)] TimerCallback callback_func);
    public static unowned Hook hook_signal(string signal_name, [CCode (delegate_target_pos = 2.9)] SignalCallback callback_func);
    public static unowned Hook hook_config(string config_pattern, [CCode (delegate_target_pos = 2.9)] ConfigCallback callback_func);
    public static void unhook(Hook hook);

    public Buffer current_buffer();

    public enum Rc
    {
        OK,
        OK_EAT,
        ERROR
    }

    [CCode (cname = "struct t_weechat_plugin")]
    public class Plugin
    {
    }

    [CCode (cname = "struct t_hook", ref_function = "", unref_function = "")]
    public class Hook
    {
    }

    namespace Config
    {
        public enum OptionSet
        {
            OK_CHANGED,
            OK_SAME_VALUE,
            OPTION_NOT_FOUND,
            ERROR
        }
        public unowned string get_plugin(string name);
        public unowned int set_plugin(string name, string new_value);
        public unowned bool is_set_plugin(string name);
    }

    [CCode (cname = "struct t_gui_buffer", ref_function = "", unref_function = "")]
    public class Buffer
    {
        public unowned string get_string(string property);
    }
}
