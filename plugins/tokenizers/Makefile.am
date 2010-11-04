INCLUDES = 			\
	-I$(top_builddir)	\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/lib

AM_LDFLAGS =					\
	-avoid-version				\
	-module					\
	-no-undefined

LIBS =						\
	$(top_builddir)/lib/libgroonga.la

tokenizer_plugins_LTLIBRARIES =
if WITH_MECAB
tokenizer_plugins_LTLIBRARIES += mecab.la
endif

mecab_la_CPPFLAGS = $(MECAB_CPPFLAGS)
mecab_la_SOURCES = mecab.c
mecab_la_LIBADD = $(LIBS) $(MECAB_LIBS)
mecab_la_LDFLAGS = $(AM_LDFLAGS) $(MECAB_LDFLAGS)
