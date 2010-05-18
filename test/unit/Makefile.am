SUBDIRS =		\
	lib		\
	fixtures	\
	util		\
	core		\
	memcached	\
	http		\
	story

TESTS = run-test.sh
TESTS_ENVIRONMENT =				\
	NO_MAKE=yes				\
	CUTTER="$(CUTTER)"			\
	CUTTER_VERBOSE=yes			\
	RUBY="$(RUBY)"				\
	GROONGA="$(GROONGA)"

if WITH_CUTTER
noinst_LTLIBRARIES = suite-groonga-test.la
endif

EXTRA_DIST =					\
	run-test.sh				\
	run-test.rb

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

if WITH_RUBY
echo-ruby:
	@echo $(RUBY)
else
echo-ruby:
endif

echo-groonga:
	@echo $(GROONGA)
