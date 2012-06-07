if WITH_CUTTER
noinst_LTLIBRARIES =				\
	test-stress-hash.la			\
	test-stress-patricia-trie.la
endif

INCLUDES =				\
	-I$(srcdir)			\
	-I$(top_srcdir)/test/unit/lib	\
	-I$(top_srcdir)/include		\
	-I$(top_srcdir)/lib		\
	$(GROONGA_INCLUDEDIR)

AM_CFLAGS =					\
	$(GCUTTER_CFLAGS)			\
	$(TEST_CFLAGS)

AM_LDFLAGS =					\
	-module					\
	-rpath $(libdir)			\
	-avoid-version				\
	-no-undefined

LIBS =								\
	$(top_builddir)/lib/libgroonga.la			\
	$(GCUTTER_LIBS)						\
	$(top_builddir)/test/unit/lib/libgrn-test-utils.la

test_stress_hash_la_SOURCES = test-stress-hash.c
test_stress_patricia_trie_la_SOURCES = test-stress-patricia-trie.c
