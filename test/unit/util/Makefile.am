if WITH_CUTTER
noinst_LTLIBRARIES =				\
	test-snip.la				\
	test-string.la
endif

INCLUDES =			\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/lib	\
	$(GROONGA_INCLUDEDIR)

AM_CFLAGS = $(GCUTTER_CFLAGS)
AM_LDFLAGS = -module -rpath $(libdir) -avoid-version

LIBS =						\
	$(top_builddir)/lib/libgroonga.la	\
	$(GCUTTER_LIBS)				\
	$(builddir)/../lib/libgrn-test-utils.la

test_snip_la_SOURCES = test-snip.c
test_string_la_SOURCES = test-string.c
