DISTRIBUTIONS = debian ubuntu
CHROOT_BASE = /var/lib/chroot
ARCHITECTURES = i386 amd64
CODES = squeeze wheezy unstable lucid natty oneiric precise
REPOSITORY_PACKAGE = groonga-keyring
REPOSITORY_VERSION = 2012.04.29
REPOSITORY_BASE_NAME = $(REPOSITORY_PACKAGE)-$(REPOSITORY_VERSION)

all:

release: build sign-packages update-repository sign-repository upload

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

sign-packages:
	./sign-packages.sh '$(GPG_UID)' '$(CODES)'

update-repository:
	./update-repository.sh '$(PACKAGE_NAME)' '$(ARCHITECTURES)' '$(CODES)'

sign-repository:
	./sign-repository.sh '$(GPG_UID)' '$(CODES)'

upload: ensure-rsync-path
	for distribution in $(DISTRIBUTIONS); do		\
	  (cd packages/$${distribution};			\
	   rsync -avz --delete					\
	     dists pool $(RSYNC_PATH)/$${distribution});	\
	done

build: build-package-deb build-repository-deb

build-package-deb: source
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

ensure-public-key:
	gpg --list-keys '$(GPG_UID)' > /dev/null ||			\
	  gpg --keyserver keyserver.ubuntu.com --recv-key '$(GPG_UID)'

$(REPOSITORY_BASE_NAME).tar.gz: ensure-public-key
	rm -rf $(REPOSITORY_BASE_NAME)
	mkdir -p $(REPOSITORY_BASE_NAME)
	gpg --armor --export '$(GPG_UID)' > \
	  $(REPOSITORY_BASE_NAME)/groonga-keyring.gpg
	tar cvzf $(REPOSITORY_BASE_NAME).tar.gz $(REPOSITORY_BASE_NAME)
