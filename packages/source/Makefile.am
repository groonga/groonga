all:

release: upload

ensure-rsync-path:
	@if test -z "$(RSYNC_PATH)"; then				\
	  echo "--with-rsync-path configure option must be specified.";	\
	  false;							\
	fi

download: ensure-rsync-path
	rsync -avz --progress $(RSYNC_PATH)/source/groonga/ files

upload: ensure-rsync-path files/$(PACKAGE)-$(VERSION).tar.gz
	rsync -avz --progress --delete files/ $(RSYNC_PATH)/source/groonga

files/$(PACKAGE)-$(VERSION).tar.gz: $(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	mkdir -p files
	cp -p $< $@
