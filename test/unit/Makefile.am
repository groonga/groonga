SUBDIRS =		\
	lib		\
	fixtures	\
	util		\
	core

if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

noinst_LTLIBRARIES = suite-groonga-test.la
endif

INCLUDES =			\
	-I$(srcdir)		\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/lib	\
	$(GROONGA_INCLUDEDIR)

AM_LDFLAGS = -module -rpath $(libdir) -avoid-version

LIBS =						\
	$(top_builddir)/lib/libgroonga.la

suite_groonga_test_la_SOURCES = suite-groonga-test.c

echo-cutter:
	@echo $(CUTTER)
