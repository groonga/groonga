SUBDIRS =					\
	locale

BUILT_SOURCES =
CLEANFILES =

all:

include $(top_srcdir)/doc/files.am

SOURCES = $(srcdir)/source
update-examples:
	PATH=$(abs_top_builddir)/src:$(abs_top_builddir)/src/suggest:$$PATH	\
	  GRN_PLUGINS_PATH="$(abs_top_builddir)/plugins"		\
	  GRN_PLUGINS_DIR="$(abs_top_srcdir)/plugins"			\
	  GRN_RUBY_SCRIPTS_DIR="$(abs_top_srcdir)/lib/mrb/scripts"	\
	  $(RUBY) $(srcdir)/update-execution-example.rb				\
	  $(SOURCES)

files.am: update-files.rb
	update-files

update-files: $(BUILT_SOURCES)
	cd locale/en/LC_MESSAGES && $(MAKE) gettext
	cd locale/en && $(MAKE) clean html
	$(RUBY) $(srcdir)/update-files.rb

clean-html clean-text clean-markdown update-po:
	for dir in $(SUBDIRS); do		\
	  (cd $${dir} && $(MAKE) $@);		\
	done

init-locale:
	for dir in $(SUBDIRS); do		\
	  (cd $${dir} && $(MAKE) init);		\
	done

EXTRA_DIST =					\
	CMakeLists.txt				\
	files.cmake				\
	images.mk				\
	update-execution-example.rb		\
	update-files.rb				\
	$(source_files_relative_from_doc_dir)

$(source_files_relative_from_doc_dir): html

update-images:
	cd $(srcdir)/source/_static/images && \
	  $(MAKE) -f $(abs_srcdir)/images.mk update-images

if DOCUMENT_BUILDABLE
dist-hook: html
	@touch $(distdir)/build-stamp

CLEANFILES += build-stamp
else
if HAVE_BUILT_DOCUMENT
EXTRA_DIST += build-stamp
endif
endif
