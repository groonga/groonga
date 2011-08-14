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
	$(relative_theme_files)

if ENABLE_DOCUMENT
dist-hook: html man
	@touch $(distdir)/build-stamp
BUILT_SOURCES =					\
	$(relative_source_files)
nobase_dist_doc_DATA =				\
	$(relative_source_files)
else
dist-hook:
	@echo "must specify --enable-document for 'make dist'"
	@false
endif
