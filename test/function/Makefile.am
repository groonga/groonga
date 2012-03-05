SUBDIRS =					\
	fixture					\
	include					\
	suite

if WITH_FUNCTION_TEST
TESTS = run-test.sh
endif
TESTS_ENVIRONMENT =				\
	NO_MAKE=yes				\
	BUILD_DIR="$(builddir)"			\
	RUBY="$(RUBY)"				\
	GROONGA="$(GROONGA)"

EXTRA_DIST =					\
	run-test.sh
