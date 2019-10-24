REPOSITORIES_PATH = repositories
DISTRIBUTIONS = centos
ARCHITECTURES = x86_64
CENTOS_VERSIONS = 6 7

EXTRA_DIST =					\
	groonga.spec.in

build: build-release-rpm

build-release-rpm: RPM-GPG-KEY-$(PACKAGE) RPM-GPG-KEY-$(PACKAGE)-RSA4096
	./build-release-rpm.sh			\
	  $(PACKAGE)				\
	  $(REPOSITORIES_PATH)/			\
	  '$(DISTRIBUTIONS)'

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
