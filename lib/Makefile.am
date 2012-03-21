SUBDIRS = dat

lib_LTLIBRARIES = libgroonga.la

include $(top_srcdir)/version.sh
AM_CFLAGS =					\
	$(NO_STRICT_ALIASING_CFLAGS)		\
	$(COVERAGE_CFLAGS)			\
	$(GRN_CFLAGS)				\
	$(MESSAGE_PACK_CFLAGS)
DEFAULT_INCLUDES = -I$(top_builddir) -I$(top_srcdir)/include
DEFS += -D_REENTRANT -DGROONGA_VERSION=\"$(GROONGA_VERSION)\" $(GRN_DEFS)

libgroonga_la_SOURCES =				\
	com.c					\
	com.h					\
	ctx.c					\
	ctx.h					\
	ctx_impl.h				\
	dat.cpp					\
	dat.h					\
	db.c					\
	db.h					\
	expr.c					\
	geo.c					\
	geo.h					\
	groonga_in.h				\
	hash.c					\
	hash.h					\
	ii.c					\
	ii.h					\
	io.c					\
	io.h					\
	nfkc.c					\
	nfkc.h					\
	output.c				\
	output.h				\
	pat.c					\
	pat.h					\
	plugin.c				\
	plugin_in.h				\
	proc.c					\
	proc.h					\
	snip.c					\
	snip.h					\
	store.c					\
	store.h					\
	str.c					\
	str.h					\
	token.c					\
	token.h					\
	tokenizer.c				\
	util.c					\
	util.h

libgroonga_la_LDFLAGS =				\
	-version-info $(LT_VERSION_INFO)	\
	-no-undefined				\
	$(WINDOWS_LDFLAGS)

libgroonga_la_LIBADD =				\
	dat/libgrndat.la			\
	$(MESSAGE_PACK_LIBS)

EXTRA_DIST = ecmascript.c ecmascript.h ecmascript.y

CLEANFILES = *.gcno *.gcda
