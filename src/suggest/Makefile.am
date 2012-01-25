bin_PROGRAMS =

NONEXISTENT_CXX_SOURCE = nonexistent.cpp

if !PLATFORM_WIN32
bin_PROGRAMS +=					\
	groonga-suggest-create-dataset

if ENABLE_SUGGEST_LEARNER
bin_PROGRAMS +=					\
	groonga-suggest-learner			\
	groonga-suggest-httpd
noinst_LTLIBRARIES = libutil.la
endif

endif

AM_CFLAGS = -fno-strict-aliasing $(COVERAGE_CFLAGS) $(GRN_CFLAGS)
DEFS += $(GRN_DEFS)

AM_LDFLAGS = -no-undefined

DEFAULT_INCLUDES =				\
	-I$(top_builddir)			\
	-I$(srcdir)				\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/lib			\
	$(GROONGA_INCLUDEDIR)

groonga_suggest_learner_SOURCES = groonga_suggest_learner.c
nodist_EXTRA_groonga_suggest_learner_SOURCES = $(NONEXISTENT_CXX_SOURCE)
groonga_suggest_learner_CFLAGS =		\
	$(AM_CFLAGS)				\
	$(LIBEVENT_CFLAGS)			\
	$(LIBZMQ_CFLAGS)			\
	$(MESSAGE_PACK_CFLAGS)
groonga_suggest_learner_LDADD =			\
	libutil.la				\
	$(top_builddir)/lib/libgroonga.la	\
	$(LIBEVENT_LIBS)			\
	$(LIBZMQ_LIBS)				\
	$(MESSAGE_PACK_LIBS)

groonga_suggest_httpd_SOURCES = groonga_suggest_httpd.c
nodist_EXTRA_groonga_suggest_httpd_SOURCES = $(NONEXISTENT_CXX_SOURCE)
groonga_suggest_httpd_CFLAGS =			\
	$(AM_CFLAGS)				\
	$(LIBEVENT_CFLAGS)			\
	$(LIBZMQ_CFLAGS)			\
	$(MESSAGE_PACK_CFLAGS)
groonga_suggest_httpd_LDADD =			\
	libutil.la				\
	$(top_builddir)/lib/libgroonga.la	\
	$(LIBEVENT_LIBS)			\
	$(LIBZMQ_LIBS)				\
	$(MESSAGE_PACK_LIBS)

groonga_suggest_create_dataset_SOURCES = groonga_suggest_create_dataset.c
nodist_EXTRA_groonga_suggest_create_dataset_SOURCES = $(NONEXISTENT_CXX_SOURCE)
groonga_suggest_create_dataset_CFLAGS =		\
	$(AM_CFLAGS)
groonga_suggest_create_dataset_LDADD =		\
	$(top_builddir)/lib/libgroonga.la

libutil_la_SOURCES = util.c util.h
libutil_la_CFLAGS =				\
	$(AM_CFLAGS)				\
	$(LIBEVENT_CFLAGS)
