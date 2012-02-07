SUBDIRS = dat

lib_LTLIBRARIES = libgroonga.la

include $(top_srcdir)/version.sh
AM_CFLAGS =					\
	-fno-strict-aliasing			\
	$(COVERAGE_CFLAGS)			\
	$(GRN_CFLAGS)				\
	$(MESSAGE_PACK_CFLAGS)
DEFAULT_INCLUDES = -I$(top_builddir) -I$(top_srcdir)/include
DEFS += -D_REENTRANT -DGROONGA_VERSION=\"$(GROONGA_VERSION)\" $(GRN_DEFS)

libgroonga_la_SOURCES =				\
	io.c					\
	str.c					\
	nfkc.c					\
	snip.c					\
	store.c					\
	com.c					\
	ctx.c					\
	hash.c					\
	db.c					\
	pat.c					\
	dat.cpp					\
	ii.c					\
	token.c					\
	proc.c					\
	expr.c					\
	util.c					\
	plugin.c				\
	output.c				\
	geo.c

libgroonga_la_LDFLAGS =				\
	-version-info $(LT_VERSION_INFO)	\
	-no-undefined				\
	$(WINDOWS_LDFLAGS)

libgroonga_la_LIBADD =				\
	dat/libgrndat.la			\
	$(MESSAGE_PACK_LIBS)

noinst_HEADERS =				\
	com.h					\
	ctx.h					\
	ctx_impl.h				\
	dat.h					\
	db.h					\
	geo.h					\
	groonga_in.h				\
	hash.h					\
	ii.h					\
	io.h					\
	nfkc.h					\
	output.h				\
	pat.h					\
	plugin_in.h				\
	proc.h					\
	snip.h					\
	store.h					\
	str.h					\
	token.h					\
	util.h

EXTRA_DIST = ecmascript.c ecmascript.h ecmascript.y

CLEANFILES = *.gcno *.gcda
