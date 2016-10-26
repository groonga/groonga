CODE_NAMES = trusty,xenial,yakkety
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
	  --code-names '$(CODE_NAMES)' \
	  --debian-directory '$(srcdir)/../debian/' \
	  --ppa '$(LAUNCHPAD_PPA)' \
	  --pgp-sign-key '$(LAUNCHPAD_UPLOADER_PGP_KEY)'

upload: upload-precise
upload-precise: source ensure-launchpad-configuration
	rm -rf precise
	mkdir -p precise
	cp -r '$(srcdir)/../debian/' precise/
	grep -v liblz4 '$(srcdir)/../debian/control' > precise/debian/control
	./upload.rb \
	  --package '$(PACKAGE)' \
	  --version '$(VERSION)' \
	  --source-archive '$(SOURCE)' \
	  --code-names 'precise' \
	  --debian-directory 'precise/debian' \
	  --ppa '$(LAUNCHPAD_PPA)' \
	  --pgp-sign-key '$(LAUNCHPAD_UPLOADER_PGP_KEY)'

source: $(SOURCE)

$(SOURCE):
	ln -s $(abs_top_builddir)/$(PACKAGE)-$(VERSION).tar.gz $(SOURCE)
