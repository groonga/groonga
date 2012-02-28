DEFS += -D_REENTRANT $(GRN_DEFS) -DGRN_DAT_EXPORT

DEFAULT_INCLUDES =				\
	-I$(top_builddir)			\
	-I$(top_srcdir)/include			\
	-I$(top_srcdir)/lib

noinst_LTLIBRARIES = libgrndat.la

libgrndat_la_SOURCES =			\
	cursor-factory.cpp		\
	file-impl.cpp			\
	file.cpp			\
	id-cursor.cpp			\
	key-cursor.cpp			\
	predictive-cursor.cpp		\
	prefix-cursor.cpp		\
	trie.cpp

noinst_HEADERS =			\
	array.hpp			\
	base.hpp			\
	block.hpp			\
	check.hpp			\
	cursor-factory.hpp		\
	cursor.hpp			\
	dat.hpp				\
	entry.hpp			\
	file-impl.hpp			\
	file.hpp			\
	header.hpp			\
	id-cursor.hpp			\
	key-cursor.hpp			\
	key.hpp				\
	node.hpp			\
	predictive-cursor.hpp		\
	prefix-cursor.hpp		\
	string.hpp			\
	trie.hpp			\
	vector.hpp

CLEANFILES = *.gcno *.gcda
