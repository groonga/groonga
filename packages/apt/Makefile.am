REPOSITORIES_PATH = repositories
DISTRIBUTIONS = debian
ARCHITECTURES = i386 amd64
CODE_NAMES = stretch
KEYRING_PACKAGE = groonga-keyring
KEYRING_VERSION = 2018.01.29
KEYRING_BASE_NAME = $(KEYRING_PACKAGE)-$(KEYRING_VERSION)
KEYRING_FILE = groonga-archive-keyring.gpg

all:

release: download build sign-packages update-repository sign-repository upload

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

sign-packages:
	./sign-packages.sh '$(GPG_UID)' '$(REPOSITORIES_PATH)/' '$(CODE_NAMES)'

update-repository:
	./update-repository.sh '$(PACKAGE_NAME)' '$(REPOSITORIES_PATH)/' \
	  '$(ARCHITECTURES)' '$(CODE_NAMES)'

sign-repository:
	./sign-repository.sh '$(GPG_UID)' '$(REPOSITORIES_PATH)/' '$(CODE_NAMES)'

upload: ensure-rsync-path
	for distribution in $(DISTRIBUTIONS); do		\
	  (cd $(REPOSITORIES_PATH)/$${distribution};		\
	   rsync -avz --progress --delete			\
	     dists pool $(RSYNC_PATH)/$${distribution});	\
	done

build: build-package-deb build-keyring-deb

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


keyring-source: tmp/$(KEYRING_BASE_NAME).tar.gz

ensure-public-key:
	gpg --list-keys '$(GPG_UID)' > /dev/null ||			\
	  gpg --keyserver keyserver.ubuntu.com --recv-key '$(GPG_UID)'
	gpg --list-keys '$(GPG_UID_RSA4096)' > /dev/null ||			\
	  gpg --keyserver keyserver.ubuntu.com --recv-key '$(GPG_UID_RSA4096)'

tmp/$(KEYRING_BASE_NAME).tar.gz: ensure-public-key
	rm -rf $(KEYRING_BASE_NAME)
	mkdir -p $(KEYRING_BASE_NAME)
	gpg --armor --export '$(GPG_UID)' > \
	  $(KEYRING_BASE_NAME)/groonga-keyring.gpg
	gpg --armor --export '$(GPG_UID_RSA4096)' > \
	  $(KEYRING_BASE_NAME)/groonga-keyring-rsa4096.gpg
	mkdir -p tmp/
	tar cvzf tmp/$(KEYRING_BASE_NAME).tar.gz $(KEYRING_BASE_NAME)

archive-keyring: keyring-source
	rm -f $(KEYRING_FILE)
	touch $(KEYRING_FILE)
	gpg --no-default-keyring \
	  --keyring ./$(KEYRING_FILE) \
	  --import $(KEYRING_BASE_NAME)/groonga-keyring.gpg
	gpg --no-default-keyring \
	  --keyring ./$(KEYRING_FILE) \
	  --import $(KEYRING_BASE_NAME)/groonga-keyring-rsa4096.gpg
