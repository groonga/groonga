lib_LTLIBRARIES = libgroonga.la

include $(top_srcdir)/version.sh
AM_CFLAGS = -fno-strict-aliasing $(COVERAGE_CFLAGS)
AM_INCLUDES = -I. -I..
DEFS=-D_REENTRANT -DGROONGA_VERSION=\"$(GROONGA_VERSION)\"

libgroonga_la_SOURCES = io.c str.c nfkc.c snip.c query.c store.c com.c ql.c scm.c ctx.c hash.c db.c pat.c ii.c token.c proc.c

libgroonga_la_LDFLAGS = -version-info 0:0:0

noinst_HEADERS = com.h io.h ql.h nfkc.h groonga_in.h snip.h store.h str.h ctx.h hash.h db.h pat.h ii.h token.h proc.h

EXTRA_DIST = expr.c expr.h expr.y

CLEANFILES = *.gcno *.gcda
