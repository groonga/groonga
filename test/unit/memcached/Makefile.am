if WITH_CUTTER
noinst_LTLIBRARIES =				\
	test-memcached.la
endif

INCLUDES =			\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/lib	\
	$(GROONGA_INCLUDEDIR)

AM_CFLAGS = $(GCUTTER_CFLAGS)
AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -lmemcached

LIBS =								\
	$(top_builddir)/lib/libgroonga.la			\
	$(GCUTTER_LIBS)						\
	$(top_builddir)/test/unit/lib/libgrn-test-utils.la	\
	$(top_builddir)/test/unit/lib/libgrn-test-hash-utils.la

test_memcached_la_SOURCES			= test-memcached.c

ruby-bindings:
	cd bindings/ruby && \
	ruby extconf.rb && \
	make clean && make

