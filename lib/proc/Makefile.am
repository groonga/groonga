AM_CPPFLAGS =					\
	-I$(top_builddir)			\
	-I$(top_builddir)/include		\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/lib

AM_CFLAGS =					\
	$(NO_STRICT_ALIASING_CFLAGS)		\
	$(COVERAGE_CFLAGS)			\
	$(GRN_CFLAGS)				\
	$(MESSAGE_PACK_CFLAGS)			\
	$(MRUBY_CFLAGS)                         \
	$(LUAJIT_CFLAGS)

AM_CXXFLAGS =					\
	$(NO_STRICT_ALIASING_CFLAGS)		\
	$(COVERAGE_CFLAGS)			\
	$(GRN_CFLAGS)				\
	$(MESSAGE_PACK_CFLAGS)			\
	$(MRUBY_CFLAGS)				\
	$(LUAJIT_CFLAGS)			\
	$(APACHE_ARROW_CFLAGS)


noinst_LTLIBRARIES = libgrnproc.la

include c_sources.am
include cpp_sources.am
libgrnproc_la_SOURCES =				\
	$(libgrnproc_c_sources)			\
	$(libgrnproc_cpp_sources)

CLEANFILES = *.gcno *.gcda
