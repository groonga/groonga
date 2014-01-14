SUBDIRS =	\
	ruby

if WITH_CUTTER
noinst_LTLIBRARIES =			\
	libgrn-test-utils.la		\
	libgrn-test-hash-utils.la	\
	libgrn-test-server.la
endif

AM_CPPFLAGS =					\
	-I$(srcdir)				\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/lib			\
	$(GROONGA_INCLUDEDIR)

#        -DCUTTER_DISABLE_DEPRECATED
AM_CFLAGS =						\
	-DCUT_RELATIVE_PATH="\"lib\""			\
	-DGROONGA=\"$(abs_top_builddir)/src/groonga\"	\
	$(GCUTTER_CFLAGS)				\
	$(TEST_CFLAGS)

# We just need to add -rpath option to make .so. We use
# $(libdir) here but we can use other absolute path like
# $(abs_builddir).  $(abs_builddir) is more reasonable
# absolute path but it's not supported by automake < 1.10. :<
AM_LDFLAGS =					\
	-rpath $(libdir)			\
	-avoid-version				\
	-no-undefined

LIBS =						\
	$(top_builddir)/lib/libgroonga.la	\
	$(GCUTTER_LIBS)

noinst_HEADERS =				\
	grn-test-utils.h			\
	grn-test-shortcut.h			\
	grn-assertions.h			\
	grn-test-hash-factory.h			\
	grn-test-hash-assertions.h		\
	grn-test-server.h

libgrn_test_utils_la_SOURCES =	\
	grn-test-utils.c	\
	grn-assertions.c

libgrn_test_hash_utils_la_SOURCES =	\
	grn-test-hash-factory.c		\
	grn-test-hash-assertions.c
libgrn_test_hash_utils_la_LIBADD =	\
	libgrn-test-utils.la

libgrn_test_server_la_SOURCES =		\
	grn-test-server.c		\
	grn-test-server.h
libgrn_test_server_la_LIBADD =	\
	libgrn-test-utils.la
