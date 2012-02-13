INCLUDES =			\
	-I$(top_builddir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/lib

AM_CFLAGS =					\
	$(MSGPACK_CFLAGS)

AM_LDFLAGS =					\
	-avoid-version				\
	-module					\
	-no-undefined

LIBS =						\
	$(top_builddir)/lib/libgroonga.la	\
	$(MSGPACK_LIBS)

normalizer_plugins_LTLIBRARIES =
if WITH_NFKC
normalizer_plugins_LTLIBRARIES += nfkc.la
endif

nfkc_la_SOURCES =				\
	nfkc.c					\
	nfkc-unicode-5.1.c
noinst_HEADERS =				\
	nfkc-unicode-5.1.h
