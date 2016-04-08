AC_PREREQ([2.65])
AC_INIT([weechat-eventc],[0.1],[],[])

AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_SRCDIR([src/])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([.aux])

AM_INIT_AUTOMAKE([1.14 subdir-objects dist-xz no-dist-gzip tar-ustar foreign])

AM_SILENT_RULES([yes])

AC_PROG_CC
AM_PROG_CC_C_O
AC_DISABLE_STATIC
LT_INIT

AC_PROG_INSTALL


glib_min_major="2"
glib_min_minor="40"
glib_min_version="${glib_min_major}.${glib_min_minor}"

weechat_min_version="1.5"


AM_CFLAGS="-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable"
AM_LDFLAGS=""

#
# Check for glib-2.0
#
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= ${glib_min_version}])
AC_DEFINE_UNQUOTED([GLIB_VERSION_MIN_REQUIRED], [(G_ENCODE_VERSION(${glib_min_major},${glib_min_minor}))], [The lower GLib version supported])

#
# Check for libeventc
#
PKG_CHECK_MODULES([LIBEVENTC], [libeventc-light libeventd])

#
# Check for weechat
#
PKG_CHECK_MODULES([WEECHAT], [weechat >= ${weechat_min_version}])

AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_LDFLAGS])

AC_CONFIG_FILES([
    Makefile
])
AC_OUTPUT
