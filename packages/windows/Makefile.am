SUBDIRS = patches
EXTRA_DIST = Rakefile

all:

release: build upload

ensure-rsync-path:
	@if test -z "$(RSYNC_PATH)"; then				\
	  echo "--with-rsync-path configure option must be specified.";	\
	  false;							\
	fi

download: ensure-rsync-path
	rsync -avz $(RSYNC_PATH)/windows/groonga/ files

upload: ensure-rsync-path
	rsync -avz --delete files/ $(RSYNC_PATH)/windows/groonga

build: source
	$(RUBY) -S rake build VERSION=$(VERSION) SOURCE=$(SOURCE)

build-groonga: source
	$(RUBY) -S rake build:groonga VERSION=$(VERSION) SOURCE=$(SOURCE)

SOURCE=../$(PACKAGE)-$(VERSION).tar.gz

source: $(SOURCE)

$(SOURCE):
	ln -s $(abs_top_builddir)/$(PACKAGE)-$(VERSION).tar.gz ../
