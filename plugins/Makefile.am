SUBDIRS =					\
	functions				\
	language_model				\
	query_expanders				\
	ruby					\
	sharding				\
	suggest					\
	token_filters				\
	tokenizers

EXTRA_DIST =					\
	CMakeLists.txt

if WITH_MRUBY
dist_plugins_DATA =				\
	$(ruby_scripts)
endif

include ruby_scripts.am
