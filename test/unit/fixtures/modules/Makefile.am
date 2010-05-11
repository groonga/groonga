INCLUDES = 			\
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/lib

AM_LDFLAGS =					\
	-avoid-version				\
	-module					\
	-no-undefined				\
	-rpath $(libdir)

LIBS =						\
	$(top_builddir)/lib/libgroonga.la

noinst_LTLIBRARIES = string.la

string_la_SOURCES = string.c
