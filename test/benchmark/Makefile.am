SUBDIRS =					\
	fixtures				\
	lib

if WITH_BENCHMARK
noinst_PROGRAMS =		\
	bench-table-factory	\
	bench-geo-distance	\
	bench-geo-select
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
bench_geo_select_SOURCES = bench-geo-select.c

benchmarks =					\
	run-bench-table-factory			\
	run-bench-geo-distance			\
	run-bench-geo-select

run-bench-table-factory: bench-table-factory
	@echo $@:
	./bench-table-factory

run-bench-geo-distance: bench-geo-distance
	@echo $@:
	./bench-geo-distance

run-bench-geo-select: bench-geo-select
	@echo $@:
	env					\
	  RUBY="$(RUBY)"			\
	  GROONGA="$(GROONGA)"			\
	  srcdir="$(srcdir)"			\
	  $(srcdir)/bench-geo-select.sh

benchmark: $(benchmarks)
