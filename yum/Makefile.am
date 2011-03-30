DISTRIBUTIONS = fedora centos
ARCHITECTURES = x86_64
CHROOT_BASE = /var/lib/chroot

all:

release: build sign update upload

remove-existing-packages:
	for distribution in $(DISTRIBUTIONS); do	\
	  find $${distribution} -name "*.rpm" -delete;	\
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

sign:
	./sign-rpm.sh '$(DISTRIBUTIONS)'

update:
	./update-repository.sh '$(DISTRIBUTIONS)'

upload: ensure-rsync-path
	for distribution in $(DISTRIBUTIONS); do		\
	  rsync -avz --delete --exclude .gitignore		\
	    $${distribution}/ $(RSYNC_PATH)/$${distribution};	\
	done

build: build-in-chroot build-repository-rpm

build-in-chroot:
	./build-in-chroot.sh			\
	  $(PACKAGE) $(VERSION) $(CHROOT_BASE)	\
	  '$(ARCHITECTURES)' '$(DISTRIBUTIONS)'

build-repository-rpm: RPM-GPG-KEY-groonga
	./build-repository-rpm.sh $(PACKAGE) '$(DISTRIBUTIONS)'

RPM-GPG-KEY-groonga:
	./gpg-public-key.sh > $@
