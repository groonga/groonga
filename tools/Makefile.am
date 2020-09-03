toolsdir = $(pkgdatadir)/tools
dist_tools_DATA =				\
	copy-related-files.rb			\
	check-index-merge-source-dump.rb	\
	check-small-index-limit.rb		\
	extract-index-source-from-dump.rb	\
	generate-long-posting-list-data.rb	\
	generate-many-segment-data.rb		\
	groonga-benchmark-indexing.rb		\
	groonga-memory-leak-checker.rb		\
	groonga-memory-usage-analyzer.rb	\
	groonga-object-list-checker.rb		\
	index-column-diff-all.sh		\
	object-inspect-all.sh			\
	prepare-sphinx-html.rb			\
	ucd-symbol-list.rb

NONEXISTENT_CXX_SOURCE = nonexistent.cpp

DEFAULT_INCLUDES =				\
	-I$(top_builddir)			\
	-I$(top_builddir)/include		\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/lib			\
	$(GROONGA_INCLUDEDIR)

noinst_PROGRAMS = create-hash-table-with-duplicated-key
create_hash_table_with_duplicated_key_SOURCES =	\
	create-hash-table-with-duplicated-key.c
nodist_EXTRA_create_hash_table_with_duplicated_key_SOURCES =	\
	$(NONEXISTENT_CXX_SOURCE)
create_hash_table_with_duplicated_key_LDADD =	\
	$(top_builddir)/lib/libgroonga.la
