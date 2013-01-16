DEFAULT_INCLUDES =					\
	-I$(srcdir)/../mruby-$(MRUBY_VERSION)/include	\
	-I$(srcdir)/../mruby-$(MRUBY_VERSION)/src

if WITH_MRUBY
noinst_LTLIBRARIES = libmruby.la

AM_CFLAGS =					\
	-DDISABLE_GEMS

include sources.am

CLEANFILES = *.gcno *gcda
endif

update:
	RUBY="$(RUBY)" ./update.sh
