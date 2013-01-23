EXTRA_DIST =					\
	CMakeLists.txt

INCLUDES =			\
	-I$(top_builddir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/lib

AM_LDFLAGS =					\
	-avoid-version				\
	-module					\
	-no-undefined

LIBS =						\
	$(top_builddir)/lib/libgroonga.la

normalizers_plugins_LTLIBRARIES =
normalizers_plugins_LTLIBRARIES += mysql.la

include mysql_sources.am
