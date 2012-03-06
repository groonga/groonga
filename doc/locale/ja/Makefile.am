LOCALE = ja

include $(top_srcdir)/build/makefiles/locale.am

am_dist_ja_man1_DATA_DIST =
if DOCUMENT_AVAILABLE
ja_man1dir = $(mandir)/ja/man1
dist_ja_man1_DATA = $(man_files)
am_dist_ja_man1_DATA_DIST +=			\
	$(dist_ja_man1_DATA)
endif
