project('weechat-eventc', 'c',
    version: '0.1',
    meson_version: '>=0.39.1',
    license: [ 'GPL3+' ],
    default_options: [
        'c_std=gnu11',
        'warning_level=2',
    ],
)

flags = [
    '-Wformat=2',
    '-Wno-unused-parameter',
]
c_compiler = meson.get_compiler('c')
foreach f : flags
    if c_compiler.has_argument(f)
        add_project_arguments(f, language: 'c')
    endif
endforeach

glib_min_major='2'
glib_min_minor='40'
glib_min_version='.'.join([glib_min_major, glib_min_minor])
eventd_min_version='0.21.90'
weechat_min_version='1.7'
glib = dependency('glib-2.0', version: '>= @0@'.format(glib_min_version))
libeventd = dependency('libeventd', version: '>= @0@'.format(eventd_min_version))
libeventc_light = dependency('libeventc-light', version: '>= @0@'.format(eventd_min_version))
weechat = dependency('weechat', version: '>= @0@'.format(weechat_min_version))


header_conf = configuration_data()
header_conf.set_quoted('PACKAGE_NAME', meson.project_name())
header_conf.set_quoted('PACKAGE_VERSION', meson.project_version())
header_conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())

header_conf.set('GLIB_VERSION_MIN_REQUIRED', '(G_ENCODE_VERSION(@0@,@1@))'.format(glib_min_major, glib_min_minor))
header_conf.set('G_LOG_USE_STRUCTURED', true)

config_h = configure_file(output: 'config.h', configuration: header_conf)


shared_module('eventc', [
        'src/plugin.c',
        config_h,
    ],
    c_args: [ '-DG_LOG_DOMAIN="weechat-eventc"' ],
    name_prefix: '',
    dependencies: [ weechat, libeventc_light, libeventd, glib ],
    install: true,
    install_dir: join_paths(get_option('libdir'), 'weechat', 'plugins')
)
