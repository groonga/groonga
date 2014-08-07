ALL_DEPENDENCIES =
CLEAN_DEPENDENCIES =
if WITH_MRUBY
ALL_DEPENDENCIES += onigmo-all
CLEAN_DEPENDENCIES += onigmo-clean
endif

onigmo-all:
	cd ../onigmo-source && $(MAKE) all

all: $(ALL_DEPENDENCIES)

onigmo-clean:
	cd ../onigmo-source && $(MAKE) clean

clean: $(CLEAN_DEPENDENCIES)
