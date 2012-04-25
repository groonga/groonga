DISTRIBUTIONS = debian ubuntu
CHROOT_BASE = /var/lib/chroot
ARCHITECTURES = i386 amd64
CODES = squeeze wheezy unstable lucid natty oneiric
REPOSITORY_PACKAGE = groonga-repository
REPOSITORY_VERSION = 1.0.0
REPOSITORY_BASE_NAME = $(REPOSITORY_PACKAGE)-$(REPOSITORY_VERSION)

all:

release: build update sign upload

remove-existing-packages:
	for distribution in $(DISTRIBUTIONS); do		\
	  find packages/$${distribution}/pool -type f -delete;	\
	done

ensure-rsync-path:
	@if test -z "$(RSYNC_PATH)"; then				\
	  echo "--with-rsync-path configure option must be specified.";	\
	  false;							\
	fi

download: ensure-rsync-path
	for distribution in $(DISTRIBUTIONS); do	\
	  rsync -avz --delete				\
	    $(RSYNC_PATH)/$${distribution} packages/;	\
	done

update:
	./update-repository.sh '$(PACKAGE_NAME)' '$(ARCHITECTURES)' '$(CODES)'

sign:
	./sign-repository.sh '$(GPG_UID)' '$(CODES)'

upload: ensure-rsync-path
	for distribution in $(DISTRIBUTIONS); do		\
	  (cd packages/$${distribution};			\
	   rsync -avz --delete					\
	     dists pool $(RSYNC_PATH)/$${distribution});	\
	done

build: build-in-chroot build-repository-deb

build-in-chroot: source
	./build-in-chroot.sh \
	  $(PACKAGE) $(VERSION) $(srcdir)/.. $(CHROOT_BASE) \
	  '$(ARCHITECTURES)' '$(CODES)'

build-repository-deb: repository-source
	./build-in-chroot.sh \
	  $(REPOSITORY_PACKAGE) $(REPOSITORY_VERSION) $(srcdir) $(CHROOT_BASE) \
	  '$(ARCHITECTURES)' '$(CODES)'

source: ../$(PACKAGE)-$(VERSION).tar.gz

../$(PACKAGE)-$(VERSION).tar.gz:
	ln -s $(abs_top_builddir)/$(PACKAGE)-$(VERSION).tar.gz ../

repository-source: $(REPOSITORY_BASE_NAME).tar.gz

$(REPOSITORY_BASE_NAME).tar.gz: $(top_srcdir)/gpg_uid
	rm -rf $(REPOSITORY_BASE_NAME)
	mkdir -p $(REPOSITORY_BASE_NAME)
	gpg --armor --export '$(GPG_UID)' > \
	  $(REPOSITORY_BASE_NAME)/groonga-keyring.gpg
	cp groonga.list.in $(REPOSITORY_BASE_NAME)/
	tar cvzf $(REPOSITORY_BASE_NAME).tar.gz $(REPOSITORY_BASE_NAME)
