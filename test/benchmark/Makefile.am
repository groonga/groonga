SUBDIRS =					\
	fixtures				\
	lib

if WITH_BENCHMARK
noinst_PROGRAMS =		\
	bench-table-factory	\
	bench-geo-distance
endif

INCLUDES =			\
	-I$(srcdir)		\
	-I$(srcdir)/lib		\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/lib	\
	$(GROONGA_INCLUDEDIR)

AM_CFLAGS =		\
	$(GLIB_CFLAGS)

LIBS =								\
	$(top_builddir)/lib/libgroonga.la			\
	$(top_builddir)/test/benchmark/lib/libbenchmark.la	\
	$(GLIB_LIBS)

bench_table_factory_SOURCES = bench-table-factory.c
bench_geo_distance_SOURCES = bench-geo-distance.c

benchmark:
	@for benchmark in $(noinst_PROGRAMS); do	\
	  echo $${benchmark}:;				\
	  ./$${benchmark};				\
	done
