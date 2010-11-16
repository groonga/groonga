lib_LTLIBRARIES = libgroonga.la

include $(top_srcdir)/version.sh
AM_CFLAGS = -fno-strict-aliasing $(COVERAGE_CFLAGS) $(GROONGA_CFLAGS)
DEFAULT_INCLUDES = -I$(top_builddir) -I$(top_srcdir)/include
DEFS += -D_REENTRANT -DGROONGA_VERSION=\"$(GROONGA_VERSION)\" $(GROONGA_DEFS)

libgroonga_la_SOURCES = io.c str.c nfkc.c snip.c query.c store.c com.c ql.c scm.c ctx.c hash.c db.c pat.c ii.c token.c proc.c expr.c util.c plugin.c output.c geo.c

libgroonga_la_LDFLAGS =				\
	-version-info 0:0:0			\
	-no-undefined				\
	$(WINDOWS_LDFLAGS)

noinst_HEADERS = com.h io.h ql.h nfkc.h groonga_in.h snip.h store.h str.h ctx.h hash.h db.h pat.h ii.h token.h proc.h util.h plugin_in.h output.h geo.h

EXTRA_DIST = ecmascript.c ecmascript.h ecmascript.y

CLEANFILES = *.gcno *.gcda
