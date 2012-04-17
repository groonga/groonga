EXTRA_DIST =					\
	CMakeLists.txt

INCLUDES =			\
	-I$(top_builddir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/lib	\
	$(MESSAGE_PACK_CFLAGS)

AM_LDFLAGS =					\
	-avoid-version				\
	-module					\
	-no-undefined

LIBS =						\
	$(top_builddir)/lib/libgroonga.la	\
	$(MESSAGE_PACK_LIBS)

suggest_plugins_LTLIBRARIES = suggest.la

include sources.am
