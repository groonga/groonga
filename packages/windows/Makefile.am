SUBDIRS = patches
EXTRA_DIST = Rakefile

all:

release: build package installer upload

ensure-rsync-path:
	@if test -z "$(RSYNC_PATH)"; then				\
	  echo "--with-rsync-path configure option must be specified.";	\
	  false;							\
	fi

download: ensure-rsync-path
	rsync -avz $(RSYNC_PATH)/windows/groonga/ files

upload: ensure-rsync-path
	rsync -avz --delete files/ $(RSYNC_PATH)/windows/groonga

build_options =					\
	VERSION=$(VERSION)			\
	SOURCE=$(SOURCE)			\
	DEBUG_BUILD=$(DEBUG_BUILD)

build: source
	$(RUBY) -S rake build $(build_options)

build-groonga: source
	$(RUBY) -S rake build:groonga $(build_options)

LICENSE_DIR = dist/share/license
installer:
	(echo "groonga";				\
	 echo "=======";				\
	 echo;						\
	 echo "AUTHORS";				\
	 echo "-------";				\
	 echo;						\
	 cat $(LICENSE_DIR)/groonga/AUTHORS;		\
	 echo;						\
	 echo "COPYING";				\
	 echo "-------";				\
	 echo;						\
	 cat $(LICENSE_DIR)/groonga/COPYING;		\
	 echo;						\
	 echo;						\
	 echo "MeCab";					\
	 echo "=====";					\
	 echo;						\
	 echo "AUTHORS";				\
	 echo "-------";				\
	 echo;						\
	 cat $(LICENSE_DIR)/mecab/AUTHORS;		\
	 echo;						\
	 echo "COPYING";				\
	 echo "-------";				\
	 echo;						\
	 cat $(LICENSE_DIR)/mecab/COPYING;		\
	 echo;						\
	 echo "BSD";					\
	 echo "---";					\
	 echo;						\
	 echo; cat $(LICENSE_DIR)/mecab/BSD;		\
	 echo;						\
	 echo "GPL";					\
	 echo "---";					\
	 echo;						\
	 cat $(LICENSE_DIR)/mecab/GPL;			\
	 echo;						\
	 echo "LGPL";					\
	 echo "----";					\
	 echo;						\
	 cat $(LICENSE_DIR)/mecab/LGPL;			\
	 echo;						\
	 echo;						\
	 echo "NAIST-jdic";				\
	 echo "==========";				\
	 echo;						\
	 echo "AUTHORS";				\
	 echo "-------";				\
	 echo;						\
	 cat $(LICENSE_DIR)/naist-jdic/AUTHORS;		\
	 echo;						\
	 echo "COPYING";				\
	 echo "-------";				\
	 echo;						\
	 cat $(LICENSE_DIR)/naist-jdic/COPYING;		\
	) > LICENSE
	makensis setup.nsi

package:
	mkdir -p files
	rm -rf files/$(PACKAGE)-$(VERSION)
	cp -a dist files/$(PACKAGE)-$(VERSION)
	(cd files && zip -r $(PACKAGE)-$(VERSION).zip $(PACKAGE)-$(VERSION))
	rm -rf files/$(PACKAGE)-$(VERSION)

SOURCE=../$(PACKAGE)-$(VERSION).tar.gz

source: $(SOURCE)

$(SOURCE):
	ln -s $(abs_top_builddir)/$(PACKAGE)-$(VERSION).tar.gz ../
