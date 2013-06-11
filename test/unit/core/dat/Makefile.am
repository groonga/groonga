if WITH_CPPCUTTER
noinst_LTLIBRARIES =				\
	test-array.la				\
	test-base.la				\
	test-block.la				\
	test-check.la				\
	test-cursor-factory.la			\
	test-entry.la				\
	test-file.la				\
	test-header.la				\
	test-id-cursor.la			\
	test-dat.la				\
	test-dat-cursor.la			\
	test-dat-pat.la				\
	test-key-cursor.la			\
	test-key.la				\
	test-node.la				\
	test-predictive-cursor.la		\
	test-prefix-cursor.la			\
	test-string.la				\
	test-trie.la				\
	test-vector.la
endif

AM_CPPFLAGS =					\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/lib			\
	-I$(top_srcdir)/test/unit/lib		\
	$(GROONGA_INCLUDEDIR)

AM_CXXFLAGS =						\
	$(GCUTTER_CFLAGS)				\
	$(CPPCUTTER_CFLAGS)				\
	$(TEST_CXXFLAGS)				\
	-DGROONGA=\"$(abs_top_builddir)/src/groonga\"

AM_LDFLAGS =					\
	-module					\
	-rpath $(libdir)			\
	-avoid-version				\
	-no-undefined

LIBS =								\
	$(top_builddir)/lib/libgroonga.la			\
	$(GCUTTER_LIBS)						\
	$(CPPCUTTER_LIBS)					\
	$(top_builddir)/test/unit/lib/libgrn-test-utils.la	\
	$(top_builddir)/test/unit/lib/libgrn-test-hash-utils.la

noinst_HEADERS =		\
	test-string.hpp

test_array_la_SOURCES			= test-array.cpp
test_base_la_SOURCES			= test-base.cpp
test_block_la_SOURCES			= test-block.cpp
test_check_la_SOURCES			= test-check.cpp
test_cursor_factory_la_SOURCES		= test-cursor-factory.cpp
test_dat_la_SOURCES			= test-dat.cpp
test_dat_cursor_la_SOURCES		= test-dat-cursor.cpp
test_dat_pat_la_SOURCES			= test-dat-pat.cpp
test_entry_la_SOURCES			= test-entry.cpp
test_file_la_SOURCES			= test-file.cpp
test_header_la_SOURCES			= test-header.cpp
test_id_cursor_la_SOURCES		= test-id-cursor.cpp
test_key_cursor_la_SOURCES		= test-key-cursor.cpp
test_key_la_SOURCES			= test-key.cpp
test_node_la_SOURCES			= test-node.cpp
test_predictive_cursor_la_SOURCES	= test-predictive-cursor.cpp
test_prefix_cursor_la_SOURCES		= test-prefix-cursor.cpp
test_string_la_SOURCES			= test-string.cpp
test_trie_la_SOURCES			= test-trie.cpp
test_vector_la_SOURCES			= test-vector.cpp
