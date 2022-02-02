LOCALE = ja

include $(top_srcdir)/build/makefiles/locale.am

am__dist_ja_man1_DATA_DIST =
if DOCUMENT_AVAILABLE
nobase_dist_doc_locale_DATA +=			\
	html/_static/translations.js
ja_man1dir = $(mandir)/ja/man1
dist_ja_man1_DATA = $(man_files)
am__dist_ja_man1_DATA_DIST +=			\
	$(dist_ja_man1_DATA)
endif
