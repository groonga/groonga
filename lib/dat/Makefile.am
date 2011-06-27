noinst_LTLIBRARIES = libgrndat.la

DEFS += -D_REENTRANT $(GRN_DEFS)

libgrndat_la_SOURCES =				\
	common-prefix-search-cursor.cpp		\
	cursor-factory.cpp			\
	id-cursor.cpp				\
	key-cursor.cpp				\
	predictive-search-cursor.cpp		\
	trie.cpp

noinst_HEADERS =				\
	base.hpp				\
	block.hpp				\
	check.hpp				\
	common-prefix-search-cursor.hpp		\
	cursor-factory.hpp			\
	cursor.hpp				\
	dat.hpp					\
	header.hpp				\
	id-cursor.hpp				\
	key-cursor.hpp				\
	key-info.hpp				\
	key.hpp					\
	node.hpp				\
	predictive-search-cursor.hpp		\
	timer.hpp				\
	trie.hpp				\
	usage.hpp

CLEANFILES = *.gcno *.gcda
