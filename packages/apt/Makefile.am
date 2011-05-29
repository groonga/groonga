DISTRIBUTIONS = debian ubuntu
CHROOT_BASE = /var/lib/chroot
ARCHITECTURES = amd64
CODES = squeeze wheezy unstable lucid natty

all:

release: build update sign upload

remove-existing-packages:
	for distribution in $(DISTRIBUTIONS); do	\
	  find $${distribution}/pool -type f -delete;	\
	done

ensure-rsync-path:
	@if test -z "$(RSYNC_PATH)"; then				\
	  echo "--with-rsync-path configure option must be specified.";	\
	  false;							\
	fi

download: ensure-rsync-path
	for distribution in $(DISTRIBUTIONS); do			\
	  rsync -avz $(RSYNC_PATH)/$${distribution}/ $${distribution};	\
	done

update:
	./update-repository.sh '$(PACKAGE_NAME)' '$(ARCHITECTURES)' '$(CODES)'

sign:
	./sign-repository.sh '$(CODES)'

upload: ensure-rsync-path
	for distribution in $(DISTRIBUTIONS); do		\
	  (cd $${distribution};					\
	   rsync -avz --delete					\
	     dists pool $(RSYNC_PATH)/$${distribution});	\
	done

build: source
	./build-in-chroot.sh \
	  $(PACKAGE) $(VERSION) $(CHROOT_BASE) \
	  '$(ARCHITECTURES)' '$(CODES)'

source: ../$(PACKAGE)-$(VERSION).tar.gz

../$(PACKAGE)-$(VERSION).tar.gz:
	ln -s $(abs_top_builddir)/$(PACKAGE)-$(VERSION).tar.gz ../
