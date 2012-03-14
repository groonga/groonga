SUBDIRS =					\
	fixture					\
	suite

if WITH_FUNCTION_TEST
TESTS = run-test.sh
endif
TESTS_ENVIRONMENT =								\
	NO_MAKE=yes								\
	BUILD_DIR="$(builddir)"							\
	RUBY="$(RUBY)"								\
	GROONGA="$(GROONGA)"							\
	GROONGA_SUGGEST_CREATE_DATASET="$(GROONGA_SUGGEST_CREATE_DATASET)"

EXTRA_DIST =					\
	run-test.sh

update-files:
	cd $(srcdir) && ./update-files.sh > files.am
