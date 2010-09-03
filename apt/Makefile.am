DISTRIBUTIONS = debian ubuntu
CHROOT_BASE = /var/lib/chroot
ARCHITECTURES = i386 amd64
CODES = lenny squeeze unstable hardy lucid

all:

release: remove-existing-packages build update sign upload

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
	for distribution in $(DISTRIBUTIONS); do			\
	  (cd $${distribution};						\
	   rsync -avz dists pool $(RSYNC_PATH)/$${distribution};	\
	  );								\
	done

rsync:
	for distribution in $(DISTRIBUTIONS); do			\
	  (cd $${distribution};						\
	   rsync -avz $(RSYNC_PATH)/$${distribution}/pool/ pool;	\
	  );								\
	done

build:
	./build-in-chroot.sh \
	  $(PACKAGE) $(VERSION) $(CHROOT_BASE) \
	  '$(ARCHITECTURES)' '$(CODES)'
