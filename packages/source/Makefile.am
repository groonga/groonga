SUFFIX =

FILES =						\
	files/$(PACKAGE)-$(VERSION).tar.gz	\
	files/$(PACKAGE)-$(VERSION).zip

TMP_FILES =						\
	files/$(PACKAGE)-$(VERSION)$(SUFFIX).tar.gz	\
	files/$(PACKAGE)-$(VERSION)$(SUFFIX).zip

all:

release: download upload

ensure-rsync-path:
	@if test -z "$(RSYNC_PATH)"; then				\
	  echo "--with-rsync-path configure option must be specified.";	\
	  false;							\
	fi

ensure-signed-archive: $(FILES)
	for file in $(notdir $(FILES)); do				\
	  (cd files &&							\
	    gpg2 --local-user $(GPG_UID_RSA4096) --armor --detach-sign $$file);	\
	done

ensure-latest-archive: $(FILES)
	cd files &&							\
	ln -sf $(PACKAGE)-$(VERSION).zip $(PACKAGE)-latest.zip;		\
	ln -sf $(PACKAGE)-$(VERSION).tar.gz $(PACKAGE)-latest.tar.gz

download: ensure-rsync-path
	rsync -avz --progress --delete $(RSYNC_PATH)/source/groonga/ files

upload: ensure-rsync-path $(FILES) ensure-signed-archive ensure-latest-archive
	rsync -avz --progress --delete files/ $(RSYNC_PATH)/source/groonga

upload-to-tmp: $(TMP_FILES)
	scp $(TMP_FILES) packages@packages.groonga.org:public/tmp/

files/$(PACKAGE)-$(VERSION)$(SUFFIX).tar.gz: $(abs_top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	mkdir -p files
	rm -rf files/$(PACKAGE)-$(VERSION)$(SUFFIX)
	cd files && tar xzf "$<"
	if [ -n "$(SUFFIX)" ]; then					\
	  cd files;							\
	  mv $(PACKAGE)-$(VERSION) $(PACKAGE)-$(VERSION)$(SUFFIX);	\
	fi
	cd files &&					\
	  tar czf $(PACKAGE)-$(VERSION)$(SUFFIX).tar.gz	\
	  $(PACKAGE)-$(VERSION)$(SUFFIX)
	rm -rf files/$(PACKAGE)-$(VERSION)$(SUFFIX)

files/$(PACKAGE)-$(VERSION)$(SUFFIX).zip: files/$(PACKAGE)-$(VERSION)$(SUFFIX).tar.gz
	rm -rf $(PACKAGE)-$(VERSION)$(SUFFIX)
	tar xzf files/$(PACKAGE)-$(VERSION)$(SUFFIX).tar.gz
	cd $(PACKAGE)-$(VERSION)$(SUFFIX)/vendor && $(RUBY) download_lz4.rb
	cd $(PACKAGE)-$(VERSION)$(SUFFIX)/vendor && $(RUBY) download_mecab.rb
	cd $(PACKAGE)-$(VERSION)$(SUFFIX)/vendor && $(RUBY) download_message_pack.rb
	cd $(PACKAGE)-$(VERSION)$(SUFFIX)/vendor && $(RUBY) download_rapidjson.rb
	cd $(PACKAGE)-$(VERSION)$(SUFFIX)/vendor && $(RUBY) download.rb zstd
	zip -r $@ $(PACKAGE)-$(VERSION)$(SUFFIX)
	rm -rf $(PACKAGE)-$(VERSION)$(SUFFIX)
