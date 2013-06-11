if WITH_CUTTER
if WITH_LIBMEMCACHED
noinst_LTLIBRARIES =				\
	test-memcached.la
endif
endif

AM_CPPFLAGS =			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/lib	\
	$(GROONGA_INCLUDEDIR)

AM_CFLAGS =						\
	$(GCUTTER_CFLAGS)				\
	-DGROONGA=\"$(abs_top_builddir)/src/groonga\"

AM_LDFLAGS =					\
	-module					\
	-rpath $(libdir)			\
	-avoid-version				\
	-no-undefined				\
	-lmemcached

LIBS =									\
	$(top_builddir)/lib/libgroonga.la				\
	$(GCUTTER_LIBS)							\
	$(top_builddir)/test/unit/lib/libgrn-test-utils.la		\
	$(top_builddir)/test/unit/lib/libgrn-test-server.la		\
	$(top_builddir)/test/unit/lib/libgrn-test-memcached-utils.la

test_memcached_la_SOURCES			= test-memcached.c
