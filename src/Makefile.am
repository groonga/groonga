bin_PROGRAMS = groonga grntest
noinst_PROGRAMS = grnslap

AM_CFLAGS = -fno-strict-aliasing $(COVERAGE_CFLAGS) \
            -DDEFAULT_ADMIN_HTML_PATH=\"$(pkgdatadir)/admin_html\"

DEFAULT_INCLUDES = -I$(top_builddir) -I$(srcdir) -I$(top_srcdir) $(GROONGA_INCLUDEDIR)

groonga_SOURCES = groonga.c
groonga_LDADD = $(top_builddir)/lib/libgroonga.la

grnslap_SOURCES = grnslap.c
grnslap_LDADD = $(top_builddir)/lib/libgroonga.la

grntest_SOURCES = grntest.c
grntest_LDADD = $(top_builddir)/lib/libgroonga.la
