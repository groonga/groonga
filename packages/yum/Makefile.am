REPOSITORIES_PATH = repositories
DISTRIBUTIONS = centos
ARCHITECTURES = i386 x86_64
BASE_URL_PREFIX = https://packages.groonga.org
HAVE_DEVELOPMENT_BRANCH = no
CENTOS_VERSIONS = 6 7
SPEC_DIR = $(builddir)/../rpm/centos

release: download build sign-packages update-repository upload

ensure-rsync-path:
	@if test -z "$(RSYNC_PATH)"; then				\
	  echo "--with-rsync-path configure option must be specified.";	\
	  false;							\
	fi

sign-packages:
	./sign-rpm.sh '$(GPG_UID)' '$(REPOSITORIES_PATH)/' \
	  '$(DISTRIBUTIONS)'

update-repository:
	./update-repository.sh \
	  '$(GPG_UID)' \
	  '$(PACKAGE)' \
	  '$(REPOSITORIES_PATH)/' \
	  '$(DISTRIBUTIONS)'

upload: ensure-rsync-path
	for distribution in $(DISTRIBUTIONS); do		\
	  rsync -avz --progress --delete --exclude .gitignore	\
	    $(REPOSITORIES_PATH)/$${distribution}/		\
	      $(RSYNC_PATH)/$${distribution};			\
	done

download: ensure-rsync-path
	mkdir -p $(REPOSITORIES_PATH)
	for distribution in $(DISTRIBUTIONS); do	\
	   rsync -avz --progress --delete		\
	     $(RSYNC_PATH)/$${distribution}/		\
	     $(REPOSITORIES_PATH)/$${distribution};	\
	done

build: build-in-vm build-release-rpm

build-in-vm: source specs env.sh
	vagrant destroy --force
	for architecture in $(ARCHITECTURES); do		\
	  for version in $(CENTOS_VERSIONS); do			\
	    if [ $$version = 7 -a $$architecture = i386 ]; then	\
	      continue;						\
	    fi;							\
	    id=centos-$$version-$$architecture;			\
	    vagrant up $$id;					\
	    vagrant destroy --force $$id;			\
	  done;							\
	done

build-release-rpm: RPM-GPG-KEY-$(PACKAGE) RPM-GPG-KEY-$(PACKAGE)-RSA4096
	./build-release-rpm.sh			\
	  $(PACKAGE)				\
	  '$(PACKAGE_LABEL)'			\
	  $(BASE_URL_PREFIX)			\
	  $(REPOSITORIES_PATH)/			\
	  '$(DISTRIBUTIONS)'			\
	  $(HAVE_DEVELOPMENT_BRANCH)

ensure-public-key:
	gpg --list-keys '$(GPG_UID)' > /dev/null ||			\
	  gpg --keyserver keyserver.ubuntu.com --recv-key '$(GPG_UID)'

ensure-public-key-rsa4096:
	gpg --list-keys '$(GPG_UID_RSA4096)' > /dev/null ||			\
	  gpg --keyserver keyserver.ubuntu.com --recv-key '$(GPG_UID_RSA4096)'

RPM-GPG-KEY-$(PACKAGE): ensure-public-key
	gpg --armor --export '$(GPG_UID)' > $@

RPM-GPG-KEY-$(PACKAGE)-RSA4096: ensure-public-key-rsa4096
	gpg --armor --export '$(GPG_UID_RSA4096)' > $@

source: tmp/$(PACKAGE)-$(VERSION).tar.gz

tmp/$(PACKAGE)-$(VERSION).tar.gz: $(abs_top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	mkdir -p tmp/
	cp $(abs_top_builddir)/$(PACKAGE)-$(VERSION).tar.gz tmp/

$(abs_top_builddir)/$(PACKAGE)-$(VERSION).tar.gz:
	cd $(abs_top_builddir) && $(MAKE) dist

specs: tmp/centos/$(PACKAGE).spec

tmp/centos/$(PACKAGE).spec: $(SPEC_DIR)/$(PACKAGE).spec.in
	mkdir -p tmp/centos
	cd $(SPEC_DIR) && $(MAKE)
	cp $(SPEC_DIR)/$(PACKAGE).spec tmp/centos/
