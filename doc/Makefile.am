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

init-locale:
	for dir in $(SUBDIRS); do		\
	  (cd $${dir} && $(MAKE) init);		\
	done

EXTRA_DIST =					\
	$(theme_files_relative_from_doc_dir)	\
	images.mk

$(source_files_relative_from_doc_dir): html

update-images:
	cd $(srcdir)/source/_static/images && \
	  make -f $(abs_srcdir)/images.mk update-images

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
