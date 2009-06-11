if WITH_CUTTER
noinst_LTLIBRARIES =			\
	libgrn-test-utils.la		\
	libgrn-test-hash-utils.la
endif

INCLUDES =					\
	-I$(srcdir)				\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/lib			\
	$(GROONGA_INCLUDEDIR)

AM_CFLAGS =				\
        -DCUTTER_DISABLE_DEPRECATED	\
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
	grn-test-utils.h			\
	grn-assertions.h			\
	grn-test-hash-factory.h			\
	grn-test-hash-assertions.h

libgrn_test_utils_la_SOURCES =	\
	grn-test-utils.c	\
	grn-assertions.c

libgrn_test_hash_utils_la_SOURCES =	\
	grn-test-hash-factory.c		\
	grn-test-hash-assertions.c
