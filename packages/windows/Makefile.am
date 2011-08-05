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

installer:
	(echo "groonga";					\
	 echo "----";						\
	 echo "AUTHORS"; cat dist/groonga/AUTHORS;		\
	 echo "----";						\
	 echo "COPYING"; cat dist/groonga/COPYING;		\
	 echo "====";						\
	 echo "MeCab";						\
	 echo "----";						\
	 echo "AUTHORS"; cat dist/mecab/AUTHORS;		\
	 echo "----";						\
	 echo "COPYING"; cat dist/mecab/COPYING;		\
	 echo "----";						\
	 echo "BSD"; cat dist/mecab/BSD;			\
	 echo "----";						\
	 echo "GPL"; cat dist/mecab/GPL;			\
	 echo "----";						\
	 echo "LGPL"; cat dist/mecab/LGPL;			\
	 echo "====";						\
	 echo "NAIST-jdic";					\
	 echo "----";						\
	 echo "AUTHORS"; cat dist/mecab-naist-jdic/AUTHORS;	\
	 echo "----";						\
	 echo "COPYING"; cat dist/mecab-naist-jdic/COPYING;	\
	) > COPYING
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
