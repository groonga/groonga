SUBDIRS =		\
	lib		\
	fixtures	\
	util		\
	core		\
	story

if WITH_UNIT_TEST
TESTS = run-test.sh
endif
TESTS_ENVIRONMENT =				\
	NO_MAKE=yes				\
	BUILD_DIR="$(builddir)"			\
	CUTTER="$(CUTTER)"			\
	CUTTER_VERBOSE=yes			\
	GROONGA="$(GROONGA)"			\
	GROONGA_BENCHMARK="$(GROONGA_BENCHMARK)"

if WITH_CUTTER
noinst_LTLIBRARIES = suite-groonga-test.la
else
TESTS_ENVIRONMENT += NO_CUTTER=yes
endif

EXTRA_DIST =					\
	run-test.sh

AM_CPPFLAGS =			\
	-I$(srcdir)		\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/lib	\
	$(GROONGA_INCLUDEDIR)

AM_LDFLAGS = -module -rpath $(libdir) -avoid-version

LIBS =						\
	$(top_builddir)/lib/libgroonga.la

suite_groonga_test_la_SOURCES = suite-groonga-test.c
