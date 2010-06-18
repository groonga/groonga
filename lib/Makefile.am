lib_LTLIBRARIES = libgroonga.la

include $(top_srcdir)/version.sh
AM_CFLAGS = -fno-strict-aliasing $(COVERAGE_CFLAGS)
DEFAULT_INCLUDES = -I$(top_builddir) -I$(srcdir) -I$(top_srcdir)
DEFS += -D_REENTRANT -DGROONGA_VERSION=\"$(GROONGA_VERSION)\"

libgroonga_la_SOURCES = io.c str.c nfkc.c snip.c query.c store.c com.c ql.c scm.c ctx.c hash.c db.c pat.c ii.c token.c proc.c expr.c util.c module.c output.c

libgroonga_la_LDFLAGS =				\
	-version-info 0:0:0			\
	-no-undefined				\
	$(WINDOWS_LDFLAGS)

noinst_HEADERS = com.h io.h ql.h nfkc.h groonga_in.h snip.h store.h str.h ctx.h hash.h db.h pat.h ii.h token.h proc.h util.h module.h output.h

EXTRA_DIST = ecmascript.c ecmascript.h ecmascript.y

CLEANFILES = *.gcno *.gcda
