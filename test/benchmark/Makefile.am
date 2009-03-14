SUBDIRS =	\
	lib

if WITH_BENCHMARK
noinst_PROGRAMS =		\
	bench-table-factory
endif

INCLUDES =			\
	-I$(srcdir)		\
	-I$(srcdir)/lib		\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/lib	\
	$(GROONGA_INCLUDEDIR)

AM_CFLAGS =		\
	$(GLIB_CFLAGS)

LIBS =						\
	$(top_builddir)/lib/libgroonga.la	\
	$(builddir)/lib/libbenchmark.la		\
	$(GLIB_LIBS)

bench_table_factory_SOURCES = bench-table-factory.c

benchmark:
	@for benchmark in $(noinst_PROGRAMS); do	\
	  echo $${benchmark}:;				\
	  ./$${benchmark};				\
	done
