all:

release: upload

ensure-rsync-path:
	@if test -z "$(RSYNC_PATH)"; then				\
	  echo "--with-rsync-path configure option must be specified.";	\
	  false;							\
	fi

download: ensure-rsync-path
	rsync -avz $(RSYNC_PATH)/windows/groonga/ files

upload: ensure-rsync-path
	rsync -avz --delete files/ $(RSYNC_PATH)/windows/groonga
