UBUNTU_CODE_NAMES = xenial,bionic,disco,eoan
UBUNTU_VERSIONS = 16.04,18.04,19.04,19.10
SOURCE = ../$(PACKAGE)-$(VERSION).tar.gz

all:

ensure-launchpad-configuration:
	@if test -z "$(LAUNCHPAD_UPLOADER_PGP_KEY)"; then		\
	  echo "--with-launchpad-uploader-pgp-key configure option must be specified.";	\
	  false;							\
	fi

upload: source ensure-launchpad-configuration
	./upload.rb \
	  --package '$(PACKAGE)' \
	  --version '$(VERSION)' \
	  --source-archive '$(SOURCE)' \
	  --ubuntu-code-names '$(UBUNTU_CODE_NAMES)' \
	  --ubuntu-versions '$(UBUNTU_VERSIONS)' \
	  --debian-directory '$(srcdir)/../debian/' \
	  --ppa '$(LAUNCHPAD_PPA)' \
	  --pgp-sign-key '$(LAUNCHPAD_UPLOADER_PGP_KEY)'

source: $(SOURCE)

$(SOURCE):
	ln -s $(abs_top_builddir)/$(PACKAGE)-$(VERSION).tar.gz $(SOURCE)
