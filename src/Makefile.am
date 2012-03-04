SUBDIRS = suggest

NONEXISTENT_CXX_SOURCE = nonexistent.cpp

bin_PROGRAMS = groonga grntest
noinst_PROGRAMS = grnslap

AM_CFLAGS =					\
	-fno-strict-aliasing			\
	$(COVERAGE_CFLAGS)			\
	$(GRN_CFLAGS)				\
	$(MESSAGE_PACK_CFLAGS)
DEFS += $(GRN_DEFS)

AM_LDFLAGS = -no-undefined

DEFAULT_INCLUDES =				\
	-I$(top_builddir)			\
	-I$(srcdir)				\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/include			\
	$(GROONGA_INCLUDEDIR)

groonga_SOURCES = groonga.c
nodist_EXTRA_groonga_SOURCES = $(NONEXISTENT_CXX_SOURCE)
groonga_CFLAGS = $(AM_CFLAGS) $(LIBEDIT_CFLAGS)
groonga_LDADD =					\
	$(top_builddir)/lib/libgroonga.la	\
	$(LIBEDIT_LIBS)				\
	$(MESSAGE_PACK_LIBS)

grnslap_SOURCES = grnslap.c
nodist_EXTRA_grnslap_SOURCES = $(NONEXISTENT_CXX_SOURCE)
grnslap_LDADD =					\
	$(top_builddir)/lib/libgroonga.la	\
	$(MESSAGE_PACK_LIBS)

grntest_SOURCES = grntest.c
nodist_EXTRA_grntest_SOURCES = $(NONEXISTENT_CXX_SOURCE)
grntest_LDADD =					\
	$(top_builddir)/lib/libgroonga.la	\
	$(MESSAGE_PACK_LIBS)
