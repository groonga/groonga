SUBDIRS =					\
	fixtures				\
	lib

NONEXISTENT_CXX_SOURCE = nonexistent.cpp

if WITH_BENCHMARK
noinst_PROGRAMS =		\
	bench-table-factory	\
	bench-geo-distance	\
	bench-geo-select	\
	bench-ctx-create
endif

AM_CPPFLAGS =			\
	-I$(srcdir)		\
	-I$(srcdir)/lib		\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/lib	\
	$(GROONGA_INCLUDEDIR)

AM_CFLAGS =		\
	$(GLIB_CFLAGS)

LIBS =								\
	$(top_builddir)/lib/libgroonga.la			\
	$(top_builddir)/benchmark/lib/libbenchmark.la		\
	$(GLIB_LIBS)

bench_table_factory_SOURCES = bench-table-factory.c
nodist_EXTRA_bench_table_factory_SOURCES = $(NONEXISTENT_CXX_SOURCE)

bench_geo_distance_SOURCES = bench-geo-distance.c
nodist_EXTRA_bench_geo_distance_SOURCES = $(NONEXISTENT_CXX_SOURCE)

bench_geo_select_SOURCES = bench-geo-select.c
nodist_EXTRA_bench_geo_select_SOURCES = $(NONEXISTENT_CXX_SOURCE)

bench_ctx_create_SOURCES = bench-ctx-create.c
nodist_EXTRA_bench_ctx_create_SOURCES = $(NONEXISTENT_CXX_SOURCE)

benchmarks =					\
	run-bench-table-factory			\
	run-bench-geo-distance			\
	run-bench-geo-select			\
	run-bench-ctx-create

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

run-bench-ctx-create: bench-ctx-create
	@echo $@:
	./bench-ctx-create

benchmark: $(benchmarks)
