SUBDIRS = dat

lib_LTLIBRARIES = libgroonga.la

include $(top_srcdir)/version.sh
AM_CFLAGS =					\
	$(NO_STRICT_ALIASING_CFLAGS)		\
	$(COVERAGE_CFLAGS)			\
	$(GRN_CFLAGS)				\
	$(MESSAGE_PACK_CFLAGS)
DEFAULT_INCLUDES = -I$(top_builddir) -I$(top_srcdir)/include
DEFS += -D_REENTRANT $(GRN_DEFS) -DGRN_DAT_EXPORT

include sources.am

libgroonga_la_LDFLAGS =				\
	-version-info $(LT_VERSION_INFO)	\
	-no-undefined				\
	$(WINDOWS_LDFLAGS)

libgroonga_la_LIBADD =				\
	dat/libgrndat.la			\
	$(MESSAGE_PACK_LIBS)

EXTRA_DIST =					\
	ecmascript.c				\
	ecmascript.h				\
	ecmascript.y				\
	CMakeLists.txt

CLEANFILES = *.gcno *.gcda
