SUBDIRS = dictionary

NONEXISTENT_CXX_SOURCE = nonexistent.cpp

noinst_PROGRAMS = kv

DEFAULT_INCLUDES = -I$(top_srcdir)/include $(GROONGA_INCLUDEDIR)

kv_SOURCES = kv.c
nodist_EXTRA_kv_SOURCES = $(NONEXISTENT_CXX_SOURCE)
kv_LDADD = $(top_builddir)/lib/libgroonga.la
