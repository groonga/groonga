if WITH_CUTTER
noinst_LTLIBRARIES =				\
	test-read-write.la
endif

AM_CPPFLAGS =				\
	-I$(srcdir)			\
	-I$(top_srcdir)/test/unit/lib	\
	-I$(top_srcdir)/include		\
	-I$(top_srcdir)/lib		\
	$(GROONGA_INCLUDEDIR)

AM_CFLAGS =					\
	$(GCUTTER_CFLAGS)			\
	$(TEST_CFLAGS)

AM_LDFLAGS =					\
	-module					\
	-rpath $(libdir)			\
	-avoid-version				\
	-no-undefined

LIBS =								\
	$(top_builddir)/lib/libgroonga.la			\
	$(GCUTTER_LIBS)						\
	$(top_builddir)/test/unit/lib/libgrn-test-utils.la

test_read_write_la_SOURCES = test-read-write.c
