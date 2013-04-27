AC_PREREQ([2.65])
AC_INIT([weechat-eventc],[0.1],[],[])

AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_SRCDIR([src/])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([.aux])

AM_INIT_AUTOMAKE([1.11 subdir-objects dist-xz no-dist-gzip tar-ustar foreign])

AM_SILENT_RULES([yes])

AC_PROG_CC
AM_PROG_CC_C_O
AC_DISABLE_STATIC
LT_INIT

AC_PROG_INSTALL


glib_min_major="2"
glib_min_minor="28"
glib_min_version="${glib_min_major}.${glib_min_minor}"


AM_CFLAGS="-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable"
AM_LDFLAGS=""
AM_VALAFLAGS="--target-glib=${glib_min_version}"

AM_PROG_VALAC([0.13])

#
# Check for glib-2.0
#
PKG_CHECK_MODULES(GLIB, glib-2.0)
GLIB_VALAFLAGS="--pkg glib-2.0"
AC_SUBST(GLIB_VALAFLAGS)
AC_DEFINE_UNQUOTED([GLIB_VERSION_MIN_REQUIRED], [(G_ENCODE_VERSION(${glib_min_major},${glib_min_minor}))], [The lower GLib version supported])

#
# Check for gthread-2.0
#
PKG_CHECK_MODULES(GTHREAD, gthread-2.0)

#
# Check for libeventc
#
PKG_CHECK_MODULES(LIBEVENTC, libeventc)
LIBEVENTC_VALAFLAGS="--pkg libeventc"
AC_SUBST(LIBEVENTC_VALAFLAGS)

#
# Check for weechat
#
PKG_CHECK_MODULES(WEECHAT, weechat)
WEECHAT_VALAFLAGS="--pkg weechat"
AC_SUBST(WEECHAT_VALAFLAGS)

AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug], [Enable debug]), enable_debug=$enableval, enable_debug=no)
if test x$enable_debug = xyes; then
  AC_DEFINE([DEBUG], [1], [Activate debug mode])
  AM_CFLAGS="${AM_CFLAGS} -O0 -ggdb3"
  AM_VALAFLAGS="${AM_VALAFLAGS} --debug -D DEBUG"
fi
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_LDFLAGS)
AC_SUBST(AM_VALAFLAGS)

AC_CONFIG_FILES([
  Makefile
  po/Makefile.in
])
AC_OUTPUT
