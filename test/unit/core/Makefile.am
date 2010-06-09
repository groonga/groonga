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
	test-query.la				\
	test-table.la				\
	test-column.la				\
	test-database.la			\
	test-database-create.la			\
	test-table-cursor.la			\
	test-table-select.la			\
	test-table-select-normalize.la		\
	test-expr.la				\
	test-expr-script.la			\
	test-expr-query.la			\
	test-expr-parse.la			\
	test-text.la				\
	test-cast-basic.la			\
	test-cast-table.la			\
	test-dump.la				\
	test-encoding.la			\
	test-view.la				\
	test-view-operations.la			\
	test-register.la			\
	test-function-cast.la			\
	test-load.la				\
	test-store-ja.la			\
	test-log.la				\
	test-table-sort-key.la			\
	test-inspect.la
endif

INCLUDES =			\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/lib	\
	$(GROONGA_INCLUDEDIR)

AM_CFLAGS =						\
	$(GCUTTER_CFLAGS)				\
	-DGROONGA=\"$(abs_top_builddir)/src/groonga\"

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
test_table_la_SOURCES			= test-table.c
test_column_la_SOURCES			= test-column.c
test_database_la_SOURCES		= test-database.c
test_database_create_la_SOURCES		= test-database-create.c
test_table_cursor_la_SOURCES		= test-table-cursor.c
test_table_select_la_SOURCES		= test-table-select.c
test_table_select_normalize_la_SOURCES	= test-table-select-normalize.c
test_expr_la_SOURCES			= test-expr.c
test_expr_script_la_SOURCES		= test-expr-script.c
test_expr_query_la_SOURCES		= test-expr-query.c
test_expr_parse_la_SOURCES		= test-expr-parse.c
test_text_la_SOURCES			= test-text.c
test_cast_basic_la_SOURCES		= test-cast-basic.c
test_cast_table_la_SOURCES		= test-cast-table.c
test_dump_la_SOURCES			= test-dump.c
test_encoding_la_SOURCES		= test-encoding.c
test_view_la_SOURCES			= test-view.c
test_view_operations_la_SOURCES		= test-view-operations.c
test_register_la_SOURCES		= test-register.c
test_function_cast_la_SOURCES		= test-function-cast.c
test_load_la_SOURCES			= test-load.c
test_store_ja_la_SOURCES		= test-store-ja.c
test_log_la_SOURCES			= test-log.c
test_table_sort_key_la_SOURCES		= test-table-sort-key.c
test_inspect_la_SOURCES			= test-inspect.c
