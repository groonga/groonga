EXTRA_DIST =					\
	CMakeLists.txt				\
	LEGAL					\
	build_config.rb				\
	mruby_build.rb				\
	mruby_build.timestamp			\
	version

DEFAULT_INCLUDES =						\
	-I$(builddir)/../mruby-build/host/include		\
	-I$(builddir)/mrbgems/mruby-file-stat/src		\
	-I$(builddir)/mruby-include				\
	-I$(srcdir)/../mruby-include				\
	-I$(srcdir)/../mruby-source/include			\
	-I$(srcdir)/../mruby-source/src				\
	-I$(srcdir)/../mruby-source/mrbgems/mruby-compiler/core	\
	-I$(srcdir)/../mruby-source/mrbgems/mruby-io/include	\
	-I$(srcdir)/../mruby-source/mrbgems/mruby-time/include

CFLAGS +=					\
	$(NO_FLOAT_EQUAL_CFLAGS)		\
	$(NO_BAD_FUNCTION_CAST_CFLAGS)		\
	$(NO_MISSING_FIELD_INITIALIZERS_CFLAGS)

if WITH_MRUBY
mruby_datadir = $(pkgdatadir)/mruby
mruby_data_DATA =				\
	LEGAL

noinst_LTLIBRARIES = libmruby.la

AM_CPPFLAGS =					\
	-I$(srcdir)/../onigmo-source		\
	-DHAVE_ONIGMO_H				\
	$(MRUBY_CPPFLAGS)

if PLATFORM_WIN32
AM_CPPFLAGS +=					\
	-DMRB_BUILD_AS_DLL			\
	-DMRB_CORE
endif

include sources.am
include built_sources.am
BUILT_SOURCES =					\
	$(built_sources)			\
	mrbgems/mruby-file-stat/src/config.h
libmruby_la_SOURCES += $(built_sources)
nodist_libmruby_la_SOURCES =			\
	mrbgems/mruby-file-stat/src/config.h

LEGAL: mruby_build.timestamp
mrblib.c: mruby_build.timestamp
mrbgems_init.c: mruby_build.timestamp
if HAVE_SYS_SYSMACROS_H
mrbgems/mruby-file-stat/src/config.h:
	echo "#define HAVE_SYS_SYSMACROS_H 1" > "$(builddir)/$@"
else
mrbgems/mruby-file-stat/src/config.h:
	touch "$(builddir)/$@"
endif

MRUBY_CONFIG = $(abs_srcdir)/build_config.rb
MRUBY_BUILD_DIR = $(abs_top_builddir)/vendor/mruby-build
mruby_build.timestamp: build_config.rb version
	$(RUBY) "$(srcdir)/mruby_build.rb"	\
	  "$(srcdir)/build_config.rb"		\
	  "$(srcdir)/../mruby-source"		\
	  "$(builddir)/../mruby-build"		\
	  "$(srcdir)/../onigmo-source"		\
	  "mruby_build.timestamp"

CLEANFILES = *.gcno *gcda
MAINTAINERCLEANFILES = mruby_build.timestamp
endif

dist-hook:
	for mrbgem in $(srcdir)/mrbgems/mruby-*; do			\
	  $(MKDIR_P) $(distdir)/mrbgems/$$(basename $${mrbgem});	\
	  GIT_DIR=$${mrbgem}/.git git archive --format=tar HEAD |	\
	    tar xf - -C $(distdir)/mrbgems/$$(basename $${mrbgem});	\
	done

update:
	cd "$(top_srcdir)" && $(RUBY) -S rake "dev:mruby:update"
