if WITH_CUTTER
if WITH_LIBMEMCACHED
noinst_LTLIBRARIES =				\
	test-memcached.la
endif
endif

INCLUDES =			\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/lib	\
	$(GROONGA_INCLUDEDIR)

AM_CFLAGS =						\
	$(GCUTTER_CFLAGS)				\
	-DGROONGA=\"$(abs_top_builddir)/src/groonga\"

AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -lmemcached

LIBS =								\
	$(top_builddir)/lib/libgroonga.la			\
	$(GCUTTER_LIBS)						\
	$(top_builddir)/test/unit/lib/libgrn-test-utils.la	\
	$(top_builddir)/test/unit/lib/libgrn-test-server.la

test_memcached_la_SOURCES			= test-memcached.c

