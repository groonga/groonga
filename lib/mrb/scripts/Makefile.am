include sources.am

EXTRA_DIST = test/empty.rb

if WITH_MRUBY
ruby_scripts_DATA = $(RUBY_SCRIPT_FILES)
else
EXTRA_DIST += $(RUBY_SCRIPT_FILES)
endif
