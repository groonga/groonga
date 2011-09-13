SUBDIRS =					\
	locale

all:

include $(top_srcdir)/doc/files.am

update-examples:
	PATH=$(top_builddir)/src:$(top_builddir)/src/suggest:$$PATH	\
	  $(srcdir)/source/update_execution_example.py			\
	  $(srcdir)/source

files.am: update-files.sh
	update-files

update-files:
	cd locale/en/LC_MESSAGES && $(MAKE) gettext
	cd locale/en && $(MAKE) clean html
	cd $(srcdir); ./update-files.sh > files.am

man pdf clean-html update-po:
	for dir in $(SUBDIRS); do		\
	  (cd $${dir} && $(MAKE) $@);		\
	done

EXTRA_DIST =					\
	$(theme_files_relative_from_doc_dir)

$(source_files_relative_from_doc_dir): html

images =								\
	$(srcdir)/source/_static/images/geo-encode-leading-2bit.png	\
	$(srcdir)/source/_static/images/geo-encode-leading-4bit.png	\
	$(srcdir)/source/_static/images/geo-points-distance.png		\
	$(srcdir)/source/_static/images/geo-points-in-circle.png	\
	$(srcdir)/source/_static/images/geo-points-in-rectangle.png	\
	$(srcdir)/source/_static/images/geo-points-sort.png		\
	$(srcdir)/source/_static/images/geo-points.png			\
	$(srcdir)/source/_static/images/geo-search-in-circle.png	\
	$(srcdir)/source/_static/images/geo-search-in-rectangle.png

.svg.png:
	inkscape --export-dpi 90 --export-background white --export-png $@ $<

if ENABLE_DOCUMENT
dist-hook: html man
	@touch $(distdir)/build-stamp
BUILT_SOURCES =					\
	$(source_files_relative_from_doc_dir)
nobase_dist_doc_DATA =				\
	$(source_files_relative_from_doc_dir)
else
dist-hook:
	@echo "must specify --enable-document for 'make dist'"
	@false
endif
