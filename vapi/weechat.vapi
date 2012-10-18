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
    [CCode (instance_pos = 0.1)]
    public delegate int PrintCallback(Buffer? buffer, time_t date, [CCode (array_length_pos = 2.9)] string*[] tags, bool displayed, bool highlight, string prefix, string message);
    [CCode (instance_pos = 0.1)]
    public delegate int CommandCallback(Buffer? buffer, [CCode (array_length_cname = "args_lenght", array_length_pos = 1.9)] string[] args, [CCode (array_length_cname = "args_lenght", array_length_pos = 1.9)] string[] args_eol);
    [CCode (instance_pos = 0.1)]
    public delegate int TimerCallback(int remaining_calls);
    [CCode (instance_pos = 0.1)]
    public delegate int SignalCallback(string signal_name, string type_data, void *signal_data);
    [CCode (instance_pos = 0.1)]
    public delegate int ConfigCallback(string name, string @value);

    [PrintfFormat]
    public static void printf(Buffer? buffer = null, string format, ...);

    public static unowned Hook hook_print(Buffer? buffer, string? tags, string? message, bool strip_colors, [CCode (delegate_target_pos = 5.1)] PrintCallback callback_func);
    public static unowned Hook hook_command(string command, string description, string args, string args_description, string completion, [CCode (delegate_target_pos = 6.1)] CommandCallback callback_func);
    public static unowned Hook hook_timer(long interval, int align_second, int max_calls, [CCode (delegate_target_pos = 4.1)] TimerCallback callback_func);
    public static unowned Hook hook_signal(string signal_name, [CCode (delegate_target_pos = 2.1)] SignalCallback callback_func);
    public static unowned Hook hook_config(string config_pattern, [CCode (delegate_target_pos = 2.1)] ConfigCallback callback_func);
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

    [CCode (cname = "struct t_config_file", ref_function = "", unref_function = "")]
    public class Config
    {
        public enum OptionSet
        {
            OK_CHANGED,
            OK_SAME_VALUE,
            OPTION_NOT_FOUND,
            ERROR
        }

        public enum Read
        {
            OK,
            MEMORY_ERROR,
            FILE_NOT_FOUND
        }

        public enum Write
        {
            OK,
            ERROR,
            MEMORY_ERROR
        }

        public static unowned string get_plugin(string name);
        public static unowned int set_plugin(string name, string new_value);
        public static unowned bool is_set_plugin(string name);

        [CCode (instance_pos = 0.1)]
        public delegate int ReloadCallback(Config file);

        public Config(string name, [CCode (delegate_target_pos = 2.1)] ReloadCallback? callback_func = null);
        public int read();

        [CCode (cname = "struct t_config_section", cprefix = "", ref_function = "", unref_function = "")]
        public class Section
        {

            [CCode (instance_pos = 0.1)]
            public delegate int ReadCallback(Config file, Section section, string option_name, string @value);

            [CCode (instance_pos = 0.1)]
            public delegate int WriteCallback(Config file, string section_name);

            [CCode (instance_pos = 0.1)]
            public delegate int WriteDefaultCallback(Config file, string section_name);

            [CCode (instance_pos = 0.1)]
            public delegate int CreateOptionCallback(Config file, Section section, string option_name, string @value);

            [CCode (instance_pos = 0.1)]
            public delegate int DeleteOptionCallback(Config file, Section section, Option option);

            [CCode (cname = "weechat_config_new_section")]
            public Section(Config file, string name, bool user_can_add_options, bool user_can_delete_options, [CCode (delegate_target_pos = 5.1)] ReadCallback? read_callback = null, [CCode (delegate_target_pos = 6.1)] WriteCallback? write_callback = null, [CCode (delegate_target_pos = 7.1)] WriteDefaultCallback? write_default_callback = null, [CCode (delegate_target_pos = 8.1)] CreateOptionCallback? create_option_callback = null, [CCode (delegate_target_pos = 9.1)] DeleteOptionCallback? delete_option_callback = null);
        }

        [CCode (cname = "struct t_config_option", cprefix = "", ref_function = "", unref_function = "")]
        public class Option
        {
            [CCode (instance_pos = 0.1)]
            public delegate bool CheckValueCallback(Option option, string @value);

            [CCode (instance_pos = 0.1)]
            public delegate void ChangeCallback(Option option);

            [CCode (instance_pos = 0.1)]
            public delegate void DeleteCallback(Option option);

            [CCode (cname = "weechat_config_new_option")]
            public Option(Config file, Section section, string name, string type, string description, string? string_values, int min, int max, string default_value, string? @value, bool null_value_allowed, [CCode (delegate_target_pos = 12.1)] CheckValueCallback? check_value_callback = null, [CCode (delegate_target_pos = 13.1)] ChangeCallback? change_callback = null, [CCode (delegate_target_pos = 14.1)] DeleteCallback? delete_callback = null);

            [CCode (cname = "weechat_config_integer")]
            public int integer();
            [CCode (cname = "weechat_config_boolean")]
            public bool boolean();
            [CCode (cname = "weechat_config_string")]
            public unowned string @string();
            [CCode (cname = "weechat_config_color")]
            public unowned string color();
        }
    }

    [CCode (cname = "struct t_gui_buffer", ref_function = "", unref_function = "")]
    public class Buffer
    {
        [CCode (delegate_target_pos = 0.1, instance_pos = 0.2)]
        public delegate Rc InputCallback(string input_data);
        [CCode (delegate_target_pos = 0.1, instance_pos = 0.2)]
        public delegate Rc CloseCallback();

        public Buffer(string name, [CCode (delegate_target_pos = 2.1)] InputCallback? input_callback = null, [CCode (delegate_target_pos = 3.1)] CloseCallback? close_callback = null);
        public unowned string get_string(string property);
        public int get_integer(string property);
        public void @set(string property, string @value);
    }
}
