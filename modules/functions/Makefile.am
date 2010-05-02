INCLUDES = 			\
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/lib

AM_LDFLAGS =					\
	-avoid-version				\
	-module					\
	-no-undefined

LIBS =						\
	$(top_builddir)/lib/libgroonga.la

function_modules_LTLIBRARIES = geo.la

geo_la_SOURCES = geo.c
