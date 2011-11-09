if WITH_CPPCUTTER
noinst_LTLIBRARIES =				\
	test-array.la
endif

INCLUDES =					\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/lib			\
	-I$(top_srcdir)/test/unit/lib		\
	$(GROONGA_INCLUDEDIR)

AM_CXXFLAGS =						\
	$(GCUTTER_CFLAGS)				\
	$(CPPCUTTER_CFLAGS)				\
	-DGROONGA=\"$(abs_top_builddir)/src/groonga\"

AM_LDFLAGS = -module -rpath $(libdir) -avoid-version

LIBS =								\
	$(top_builddir)/lib/libgroonga.la			\
	$(GCUTTER_LIBS)						\
	$(CPPCUTTER_LIBS)					\
	$(top_builddir)/test/unit/lib/libgrn-test-utils.la	\
	$(top_builddir)/test/unit/lib/libgrn-test-hash-utils.la

test_array_la_SOURCES			= test-array.cpp
