ACLOCAL_AMFLAGS = -I m4

CLEANFILES =

AM_CFLAGS += \
	-I $(top_srcdir)/include/ \
	-I $(top_builddir)/include/ \
	--include config.h \
	$(null)

AM_VALAFLAGS += \
	--vapidir $(top_srcdir)/vapi/ \
	--pkg config

plugindir = $(libdir)/weechat/plugins

plugin_LTLIBRARIES = \
	eventc.la


eventc_la_VALASOURCES = \
	src/config.vala \
	src/eventc.vala

eventc_la_SOURCES = \
	$(eventc_la_VALASOURCES) \
	src/plugin.c

eventc_la_VALAFLAGS = \
	$(AM_VALAFLAGS) \
	$(LIBEVENTC_VALAFLAGS) \
	$(GLIB_VALAFLAGS) \
	$(WEECHAT_VALAFLAGS)

eventc_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(LIBEVENTC_CFLAGS) \
	$(GTHREAD_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(WEECHAT_CFLAGS)

eventc_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-shared \
	-avoid-version -module \
	-export-symbols-rexex weechat_plugin_*

eventc_la_LIBADD = \
	$(LIBEVENTC_LIBS) \
	$(GTHREAD_LIBS) \
	$(GLIB_LIBS) \
	$(WEECHAT_LIBS)


CLEANFILES += \
	$(eventc_la_VALASOURCES:.vala=.c)



install-exec-hook: la-files-removal

la-files-removal:
	cd $(DESTDIR)/$(plugindir) && \
		rm $(plugin_LTLIBRARIES)
