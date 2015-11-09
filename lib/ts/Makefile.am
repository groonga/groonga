DEFAULT_INCLUDES =				\
	-I$(top_builddir)			\
	-I$(top_srcdir)/include

AM_CFLAGS =					\
	$(NO_STRICT_ALIASING_CFLAGS)		\
	$(COVERAGE_CFLAGS)			\
	$(GRN_CFLAGS)

noinst_LTLIBRARIES = libgrnts.la

include sources.am

CLEANFILES = *.gcno *.gcda
