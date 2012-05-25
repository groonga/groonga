REPOSITORIES_PATH = repositories
DISTRIBUTIONS = fedora centos
ARCHITECTURES = i386 x86_64
CHROOT_BASE = /var/lib/chroot
BASE_URL_PREFIX = http://packages.groonga.org
HAVE_DEVELOPMENT_BRANCH = no
USE_RPMFORGE = no
USE_ATRPMS = no

release: download build sign update upload

ensure-rsync-path:
	@if test -z "$(RSYNC_PATH)"; then				\
	  echo "--with-rsync-path configure option must be specified.";	\
	  false;							\
	fi

sign:
	./sign-rpm.sh '$(GPG_UID)' '$(REPOSITORIES_PATH)/' \
	  '$(DISTRIBUTIONS)'

update:
	./update-repository.sh $(PACKAGE) '$(REPOSITORIES_PATH)/' \
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

build: build-in-chroot build-release-rpm

build-in-chroot: source
	./build-in-chroot.sh			\
	  $(PACKAGE)				\
	  $(VERSION)				\
	  $(top_builddir)/$(PACKAGE)		\
	  ../rpm				\
	  $(REPOSITORIES_PATH)/			\
	  $(CHROOT_BASE)			\
	  '$(ARCHITECTURES)'			\
	  '$(DISTRIBUTIONS)'			\
	  '$(HAVE_DEVELOPMENT_BRANCH)'		\
	  '$(USE_RPMFORGE)'			\
	  '$(USE_ATRPMS)'

build-release-rpm: RPM-GPG-KEY-$(PACKAGE)
	./build-release-rpm.sh			\
	  $(PACKAGE)				\
	  '$(PACKAGE_TITLE)'			\
	  $(BASE_URL_PREFIX)			\
	  $(REPOSITORIES_PATH)/			\
	  '$(DISTRIBUTIONS)'			\
	  $(HAVE_DEVELOPMENT_BRANCH)

ensure-public-key:
	gpg --list-keys '$(GPG_UID)' > /dev/null ||			\
	  gpg --keyserver keyserver.ubuntu.com --recv-key '$(GPG_UID)'

RPM-GPG-KEY-$(PACKAGE): ensure-public-key
	gpg --armor --export '$(GPG_UID)' > $@

source: ../$(PACKAGE)-$(VERSION).tar.gz

../$(PACKAGE)-$(VERSION).tar.gz: $(abs_top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	ln -s $(abs_top_builddir)/$(PACKAGE)-$(VERSION).tar.gz ../

$(abs_top_builddir)/$(PACKAGE)-$(VERSION).tar.gz:
	cd $(abs_top_builddir) && $(MAKE) dist
