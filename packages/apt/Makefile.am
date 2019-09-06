REPOSITORIES_PATH = repositories
DISTRIBUTIONS = debian
ARCHITECTURES = i386 amd64
CODE_NAMES = stretch buster

all:

release: download build upload

remove-existing-packages:
	for distribution in $(DISTRIBUTIONS); do		\
	  find $(REPOSITORIES_PATH)/$${distribution}/pool	\
	    -type f -delete;					\
	done

ensure-rsync-path:
	@if test -z "$(RSYNC_PATH)"; then				\
	  echo "--with-rsync-path configure option must be specified.";	\
	  false;							\
	fi

download: ensure-rsync-path
	for distribution in $(DISTRIBUTIONS); do	\
	  rsync -avz --progress --delete		\
	    $(RSYNC_PATH)/$${distribution}		\
	    ${REPOSITORIES_PATH}/;			\
	done

upload: ensure-rsync-path
	for distribution in $(DISTRIBUTIONS); do		\
	  (cd $(REPOSITORIES_PATH)/$${distribution};		\
	   rsync -avz --progress --delete			\
	     dists pool $(RSYNC_PATH)/$${distribution});	\
	done

build: build-package-deb

run-vagrant:
	vagrant destroy --force
	for architecture in $(ARCHITECTURES); do		\
	  for code_name in $(CODE_NAMES); do			\
	    id=debian-$$code_name-$$architecture;		\
	    vagrant up $$id || exit 1;				\
	    vagrant destroy --force $$id;			\
	  done;							\
	done

build-package-deb: prepare-build-package-deb
	$(MAKE) run-vagrant

prepare-build-package-deb: source env.sh
	cp env.sh tmp/
	rm -rf tmp/debian
	cp -rp $(srcdir)/../debian/ tmp/

source: tmp/$(PACKAGE)-$(VERSION).tar.gz

tmp/$(PACKAGE)-$(VERSION).tar.gz: $(abs_top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	mkdir -p tmp/
	cp $(abs_top_builddir)/$(PACKAGE)-$(VERSION).tar.gz $@
