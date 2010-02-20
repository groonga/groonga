bin_PROGRAMS = groonga
noinst_PROGRAMS = grnslap grntest

AM_CFLAGS = -fno-strict-aliasing $(COVERAGE_CFLAGS) \
            -DDEFAULT_ADMIN_HTML_PATH=\"$(pkgdatadir)/admin_html\"

INCLUDES = -I. -I.. $(GROONGA_INCLUDEDIR)

groonga_SOURCES = groonga.c
groonga_LDADD = $(top_builddir)/lib/libgroonga.la

grnslap_SOURCES = grnslap.c
grnslap_LDADD = $(top_builddir)/lib/libgroonga.la

grntest_SOURCES = grntest.c
grntest_LDADD = $(top_builddir)/lib/libgroonga.la
