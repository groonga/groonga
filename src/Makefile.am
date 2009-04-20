bin_PROGRAMS = groonga
noinst_PROGRAMS = grnslap

AM_CFLAGS = -fno-strict-aliasing $(COVERAGE_CFLAGS)

INCLUDES = -I. -I.. $(GROONGA_INCLUDEDIR)

groonga_SOURCES = groonga.c
groonga_LDADD = $(top_builddir)/lib/libgroonga.la

grnslap_SOURCES = grnslap.c
grnslap_LDADD = $(top_builddir)/lib/libgroonga.la
