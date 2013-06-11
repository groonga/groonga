AM_CPPFLAGS =			\
	-I$(top_builddir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/lib

AM_LDFLAGS =					\
	-avoid-version				\
	-module					\
	-no-undefined				\
	-rpath $(libdir)

LIBS =						\
	$(top_builddir)/lib/libgroonga.la

if WITH_CUTTER
noinst_LTLIBRARIES = string.la
endif

string_la_SOURCES = string.c
