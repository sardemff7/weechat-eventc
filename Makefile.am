ACLOCAL_AMFLAGS = -I m4

CLEANFILES =

plugindir = $(libdir)/weechat/plugins

plugin_LTLIBRARIES = \
	eventc.la


eventc_la_SOURCES = \
	src/plugin.c

eventc_la_CFLAGS = \
	$(AM_CFLAGS) \
	-DG_LOG_DOMAIN='"weechat-eventc"' \
	$(LIBEVENTC_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(WEECHAT_CFLAGS)

eventc_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-shared \
	-avoid-version -module \
	-export-symbols-rexex weechat_plugin_*

eventc_la_LIBADD = \
	$(LIBEVENTC_LIBS) \
	$(GLIB_LIBS) \
	$(WEECHAT_LIBS)

-include local-rules.mk

install-exec-hook: la-files-removal

la-files-removal:
	cd $(DESTDIR)/$(plugindir) && \
		rm $(plugin_LTLIBRARIES)
