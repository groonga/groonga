if WITH_CUTTER
noinst_LTLIBRARIES =				\
	test-context.la				\
	test-hash.la				\
	test-hash-sort.la			\
	test-hash-cursor.la			\
	test-inverted-index.la			\
	test-patricia-trie.la			\
	test-patricia-trie-cursor.la		\
	test-patricia-trie-search.la		\
	test-performance.la			\
	test-stress.la				\
	test-public-context.la			\
	test-query.la
endif

INCLUDES =			\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/lib	\
	$(GROONGA_INCLUDEDIR)

AM_CFLAGS = $(GCUTTER_CFLAGS)
AM_LDFLAGS = -module -rpath $(libdir) -avoid-version

LIBS =								\
	$(top_builddir)/lib/libgroonga.la			\
	$(GCUTTER_LIBS)						\
	$(top_builddir)/test/unit/lib/libgrn-test-utils.la	\
	$(top_builddir)/test/unit/lib/libgrn-test-hash-utils.la

noinst_HEADERS =		\
	test-patricia-trie.h	\
	test-hash.h

test_context_la_SOURCES			= test-context.c
test_hash_la_SOURCES			= test-hash.c
test_hash_sort_la_SOURCES		= test-hash-sort.c
test_hash_cursor_la_SOURCES		= test-hash-cursor.c
test_inverted_index_la_SOURCES		= test-inverted-index.c
test_patricia_trie_la_SOURCES		= test-patricia-trie.c
test_patricia_trie_cursor_la_SOURCES	= test-patricia-trie-cursor.c
test_patricia_trie_search_la_SOURCES	= test-patricia-trie-search.c
test_performance_la_SOURCES		= test-performance.c
test_stress_la_SOURCES			= test-stress.c
test_public_context_la_SOURCES		= test-public-context.c
test_query_la_SOURCES			= test-query.c
