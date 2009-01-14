if WITH_CUTTER
noinst_LTLIBRARIES =			\
	libsen-test-utils.la		\
	libsen-test-hash-utils.la
endif

INCLUDES =					\
	-I$(srcdir)				\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/lib			\
	$(GROONGA_INCLUDEDIR)

AM_CFLAGS =				\
	-DCUT_RELATIVE_PATH="\"lib\""	\
	$(GCUTTER_CFLAGS)

# We just need to add -rpath option to make .so. We use
# $(libdir) here but we can use other absolute path like
# $(abs_builddir).  $(abs_builddir) is more reasonable
# absolute path but it's not supported by automake < 1.10. :<
AM_LDFLAGS =                   \
       -rpath $(libdir)        \
       -avoid-version

LIBS =						\
	$(top_builddir)/lib/libgroonga.la		\
	$(GCUTTER_LIBS)

noinst_HEADERS =				\
	sen-test-utils.h			\
	sen-assertions.h			\
	sen-test-hash-factory.h			\
	sen-test-hash-assertions.h

libsen_test_utils_la_SOURCES =	\
	sen-test-utils.c	\
	sen-assertions.c

libsen_test_hash_utils_la_SOURCES =	\
	sen-test-hash-factory.c		\
	sen-test-hash-assertions.c
