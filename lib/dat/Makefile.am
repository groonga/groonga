DEFS += -D_REENTRANT $(GRN_DEFS)

if !OS_WIN32
noinst_LTLIBRARIES = libgrndat.la

libgrndat_la_SOURCES =				\
	common-prefix-cursor.cpp		\
	cursor-factory.cpp			\
	id-cursor.cpp				\
	key-cursor.cpp				\
	memory-mapped-file-impl.cpp		\
	memory-mapped-file.cpp			\
	predictive-cursor.cpp			\
	trie.cpp

noinst_HEADERS =				\
	base.hpp				\
	block.hpp				\
	check.hpp				\
	common-prefix-cursor.hpp		\
	cursor-factory.hpp			\
	cursor.hpp				\
	dat.hpp					\
	header.hpp				\
	id-cursor.hpp				\
	key-cursor.hpp				\
	key-info.hpp				\
	key.hpp					\
	memory-mapped-file-impl.hpp		\
	memory-mapped-file.hpp			\
	node.hpp				\
	predictive-cursor.hpp			\
	timer.hpp				\
	trie.hpp				\
	usage.hpp
endif

CLEANFILES = *.gcno *.gcda
