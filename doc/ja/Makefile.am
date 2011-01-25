docdir = $(pkgdatadir)/doc
doc_jadir = $(docdir)/ja

# Makefile for Sphinx documentation
#

# You can set these variables from the command line.
SPHINXOPTS    =
PAPER         =
BUILDDIR      = .
DOCTREES_BASE = $(BUILDDIR)/doctrees

# Internal variables.
PAPEROPT_a4     = -D latex_paper_size=a4
PAPEROPT_letter = -D latex_paper_size=letter
ALLSPHINXOPTS   = $(PAPEROPT_$(PAPER)) $(SPHINXOPTS) source

.PHONY: help clean man html dirhtml pickle json htmlhelp qthelp latex changes linkcheck doctest

help:
	@echo "Please use \`make <target>' where <target> is one of"
	@echo "  man       to make man files"
	@echo "  html      to make standalone HTML files"
	@echo "  dirhtml   to make HTML files named index.html in directories"
	@echo "  pickle    to make pickle files"
	@echo "  json      to make JSON files"
	@echo "  htmlhelp  to make HTML files and a HTML help project"
	@echo "  qthelp    to make HTML files and a qthelp project"
	@echo "  latex     to make LaTeX files, you can set PAPER=a4 or PAPER=letter"
	@echo "  rdoc      to make RDoc files"
	@echo "  changes   to make an overview of all changed/added/deprecated items"
	@echo "  linkcheck to check all external links for integrity"
	@echo "  doctest   to run all doctests embedded in the documentation (if enabled)"

clean-local:
	-rm -rf $(DOCTREES_BASE)
	-rm -rf $(BUILDDIR)/man
	-rm -rf $(BUILDDIR)/html
	-rm -rf $(BUILDDIR)/dirhtml
	-rm -rf $(BUILDDIR)/pickle
	-rm -rf $(BUILDDIR)/json
	-rm -rf $(BUILDDIR)/htmlhelp
	-rm -rf $(BUILDDIR)/qthelp
	-rm -rf $(BUILDDIR)/latex
	-rm -rf $(BUILDDIR)/rdoc
	-rm -rf $(BUILDDIR)/changes
	-rm -rf $(BUILDDIR)/linkcheck
	-rm -rf $(BUILDDIR)/doctest
	-rm -rf $(BUILDDIR)/pdf

man:
	$(SPHINX_BUILD) -d $(DOCTREES_BASE)/man -b man $(ALLSPHINXOPTS) $(BUILDDIR)/man
	@echo
	@echo "Build finished. The man pages are in $(BUILDDIR)/man."

html:
	$(SPHINX_BUILD) -d $(DOCTREES_BASE)/html -b html $(ALLSPHINXOPTS) $(BUILDDIR)/html
	@echo
	@echo "Build finished. The HTML pages are in $(BUILDDIR)/html."

dirhtml:
	$(SPHINX_BUILD) -b dirhtml $(ALLSPHINXOPTS) $(BUILDDIR)/dirhtml
	@echo
	@echo "Build finished. The HTML pages are in $(BUILDDIR)/dirhtml."

pickle:
	$(SPHINX_BUILD) -b pickle $(ALLSPHINXOPTS) $(BUILDDIR)/pickle
	@echo
	@echo "Build finished; now you can process the pickle files."

json:
	$(SPHINX_BUILD) -b json $(ALLSPHINXOPTS) $(BUILDDIR)/json
	@echo
	@echo "Build finished; now you can process the JSON files."

htmlhelp:
	$(SPHINX_BUILD) -b htmlhelp $(ALLSPHINXOPTS) $(BUILDDIR)/htmlhelp
	@echo
	@echo "Build finished; now you can run HTML Help Workshop with the" \
	      ".hhp project file in $(BUILDDIR)/htmlhelp."

qthelp:
	$(SPHINX_BUILD) -b qthelp $(ALLSPHINXOPTS) $(BUILDDIR)/qthelp
	@echo
	@echo "Build finished; now you can run "qcollectiongenerator" with the" \
	      ".qhcp project file in $(BUILDDIR)/qthelp, like this:"
	@echo "# qcollectiongenerator $(BUILDDIR)/qthelp/groonga.qhcp"
	@echo "To view the help file:"
	@echo "# assistant -collectionFile $(BUILDDIR)/qthelp/groonga.qhc"

latex:
	$(SPHINX_BUILD) -b latex $(ALLSPHINXOPTS) $(BUILDDIR)/latex
	@echo
	@echo "Build finished; the LaTeX files are in $(BUILDDIR)/latex."
	@echo "Run \`make all-pdf' or \`make all-ps' in that directory to" \
	      "run these through (pdf)latex."

rdoc:
	$(SPHINX_BUILD) -b rdoc $(ALLSPHINXOPTS) $(BUILDDIR)/rdoc
	@echo
	@echo "Build finished; the RDoc files are in $(BUILDDIR)/rdoc."

changes:
	$(SPHINX_BUILD) -b changes $(ALLSPHINXOPTS) $(BUILDDIR)/changes
	@echo
	@echo "The overview file is in $(BUILDDIR)/changes."

linkcheck:
	$(SPHINX_BUILD) -b linkcheck $(ALLSPHINXOPTS) $(BUILDDIR)/linkcheck
	@echo
	@echo "Link check complete; look for any errors in the above output " \
	      "or in $(BUILDDIR)/linkcheck/output.txt."

doctest:
	$(SPHINX_BUILD) -b doctest $(ALLSPHINXOPTS) $(BUILDDIR)/doctest
	@echo "Testing of doctests in the sources finished, look at the " \
	      "results in $(BUILDDIR)/doctest/output.txt."

pdf:
	$(SPHINX_BUILD) -b pdf $(ALLSPHINXOPTS) $(BUILDDIR)/pdf
	@echo
	@echo "Build finished. The PDF files are in $(BUILDDIR)/pdf."

update-examples:
	GROONGA=$(GROONGA) $(srcdir)/source/update_execution_example.py

if ENABLE_DOCUMENT
dist-hook:
	@touch $(distdir)/man-build-stamp
	@touch $(distdir)/html-build-stamp

dist_man1_MANS =				\
	man/groonga.1

$(dist_man1_MANS): man-build-stamp
man-build-stamp:
	$(MAKE) man

nobase_dist_doc_ja_DATA =			\
	$(source_files)				\
	$(html_files)

# find source -type f -not -name '*.pyc' | sort | sed -e 's,^,\t,g'
source_files =					\
	source/__init__.py							\
	source/characteristic.txt						\
	source/command_version.txt						\
	source/commands.txt							\
	source/commands/cache_limit.txt						\
	source/commands/check.txt						\
	source/commands/clearlock.txt						\
	source/commands/column_create.txt					\
	source/commands/column_list.txt						\
	source/commands/column_remove.txt					\
	source/commands/define_selector.txt					\
	source/commands/defrag.txt						\
	source/commands/delete.txt						\
	source/commands/dump.txt						\
	source/commands/load.txt						\
	source/commands/log_level.txt						\
	source/commands/log_put.txt						\
	source/commands/log_reopen.txt						\
	source/commands/quit.txt						\
	source/commands/select.txt						\
	source/commands/shutdown.txt						\
	source/commands/status.txt						\
	source/commands/suggest.txt						\
	source/commands/table_create.txt					\
	source/commands/table_list.txt						\
	source/commands/table_remove.txt					\
	source/commands/view_add.txt						\
	source/commands_not_implemented/add.txt					\
	source/commands_not_implemented/get.txt					\
	source/commands_not_implemented/set.txt					\
	source/conf.py								\
	source/developer.txt							\
	source/developer/com.txt						\
	source/developer/document.txt						\
	source/developer/query.txt						\
	source/developer/test.txt						\
	source/example/tutorial01-1.log						\
	source/example/tutorial01-10.log					\
	source/example/tutorial01-11.log					\
	source/example/tutorial01-12.log					\
	source/example/tutorial01-13.log					\
	source/example/tutorial01-14.log					\
	source/example/tutorial01-15.log					\
	source/example/tutorial01-16.log					\
	source/example/tutorial01-17.log					\
	source/example/tutorial01-2.log						\
	source/example/tutorial01-3.log						\
	source/example/tutorial01-4.log						\
	source/example/tutorial01-5.log						\
	source/example/tutorial01-6.log						\
	source/example/tutorial01-7.log						\
	source/example/tutorial01-8.log						\
	source/example/tutorial01-9.log						\
	source/example/tutorial02-1.log						\
	source/example/tutorial02-2.log						\
	source/example/tutorial02-3.log						\
	source/example/tutorial03-1.log						\
	source/example/tutorial03-2.log						\
	source/example/tutorial03-3.log						\
	source/example/tutorial04-1.log						\
	source/example/tutorial04-2.log						\
	source/example/tutorial04-3.log						\
	source/example/tutorial04-4.log						\
	source/example/tutorial04-5.log						\
	source/example/tutorial04-6.log						\
	source/example/tutorial04-7.log						\
	source/example/tutorial05-1.log						\
	source/example/tutorial05-2.log						\
	source/example/tutorial05-3.log						\
	source/example/tutorial05-4.log						\
	source/example/tutorial05-5.log						\
	source/example/tutorial05-6.log						\
	source/example/tutorial06-1.log						\
	source/example/tutorial06-2.log						\
	source/example/tutorial06-3.log						\
	source/example/tutorial06-4.log						\
	source/example/tutorial06-5.log						\
	source/example/tutorial07-1.log						\
	source/example/tutorial07-2.log						\
	source/example/tutorial07-3.log						\
	source/example/tutorial07-4.log						\
	source/example/tutorial08-1.log						\
	source/example/tutorial10-1.log						\
	source/example/tutorial10-10.log					\
	source/example/tutorial10-2.log						\
	source/example/tutorial10-3.log						\
	source/example/tutorial10-4.log						\
	source/example/tutorial10-5.log						\
	source/example/tutorial10-6.log						\
	source/example/tutorial10-7.log						\
	source/example/tutorial10-8.log						\
	source/example/tutorial10-9.log						\
	source/execfile.txt							\
	source/expr.txt								\
	source/functions.txt							\
	source/functions/edit_distance.txt					\
	source/functions/geo_distance.txt					\
	source/functions/geo_in_circle.txt					\
	source/functions/geo_in_rectangle.txt					\
	source/functions/now.txt						\
	source/functions/rand.txt						\
	source/grnslap.txt							\
	source/grntest.txt							\
	source/http.txt								\
	source/index.txt							\
	source/install.txt							\
	source/news.txt								\
	source/process.txt							\
	source/pseudo_column.txt						\
	source/rdoc.py								\
	source/reference.txt							\
	source/spec.txt								\
	source/spec/search.txt							\
	source/troubleshooting.txt						\
	source/troubleshooting/different_results_with_the_same_keyword.txt	\
	source/tutorial.txt							\
	source/tutorial/tutorial01.txt						\
	source/tutorial/tutorial02.txt						\
	source/tutorial/tutorial03.txt						\
	source/tutorial/tutorial04.txt						\
	source/tutorial/tutorial05.txt						\
	source/tutorial/tutorial06.txt						\
	source/tutorial/tutorial07.txt						\
	source/tutorial/tutorial08.txt						\
	source/tutorial/tutorial09.txt						\
	source/tutorial/tutorial10.txt						\
	source/type.txt								\
	source/update_execution_example.py

# find html -type f | sort | sed -e 's,^,\t,g'
html_files =						\
	html/.buildinfo								  \
	html/_sources/characteristic.txt					  \
	html/_sources/command_version.txt					  \
	html/_sources/commands.txt						  \
	html/_sources/commands/cache_limit.txt					  \
	html/_sources/commands/check.txt					  \
	html/_sources/commands/clearlock.txt					  \
	html/_sources/commands/column_create.txt				  \
	html/_sources/commands/column_list.txt					  \
	html/_sources/commands/column_remove.txt				  \
	html/_sources/commands/define_selector.txt				  \
	html/_sources/commands/defrag.txt					  \
	html/_sources/commands/delete.txt					  \
	html/_sources/commands/dump.txt						  \
	html/_sources/commands/load.txt						  \
	html/_sources/commands/log_level.txt					  \
	html/_sources/commands/log_put.txt					  \
	html/_sources/commands/log_reopen.txt					  \
	html/_sources/commands/quit.txt						  \
	html/_sources/commands/select.txt					  \
	html/_sources/commands/shutdown.txt					  \
	html/_sources/commands/status.txt					  \
	html/_sources/commands/suggest.txt					  \
	html/_sources/commands/table_create.txt					  \
	html/_sources/commands/table_list.txt					  \
	html/_sources/commands/table_remove.txt					  \
	html/_sources/commands/view_add.txt					  \
	html/_sources/commands_not_implemented/add.txt				  \
	html/_sources/commands_not_implemented/get.txt				  \
	html/_sources/commands_not_implemented/set.txt				  \
	html/_sources/developer.txt						  \
	html/_sources/developer/com.txt						  \
	html/_sources/developer/document.txt					  \
	html/_sources/developer/query.txt					  \
	html/_sources/developer/test.txt					  \
	html/_sources/execfile.txt						  \
	html/_sources/expr.txt							  \
	html/_sources/functions.txt						  \
	html/_sources/functions/edit_distance.txt				  \
	html/_sources/functions/geo_distance.txt				  \
	html/_sources/functions/geo_in_circle.txt				  \
	html/_sources/functions/geo_in_rectangle.txt				  \
	html/_sources/functions/now.txt						  \
	html/_sources/functions/rand.txt					  \
	html/_sources/grnslap.txt						  \
	html/_sources/grntest.txt						  \
	html/_sources/http.txt							  \
	html/_sources/index.txt							  \
	html/_sources/install.txt						  \
	html/_sources/news.txt							  \
	html/_sources/process.txt						  \
	html/_sources/pseudo_column.txt						  \
	html/_sources/reference.txt						  \
	html/_sources/spec.txt							  \
	html/_sources/spec/search.txt						  \
	html/_sources/troubleshooting.txt					  \
	html/_sources/troubleshooting/different_results_with_the_same_keyword.txt \
	html/_sources/tutorial.txt						  \
	html/_sources/tutorial/tutorial01.txt					  \
	html/_sources/tutorial/tutorial02.txt					  \
	html/_sources/tutorial/tutorial03.txt					  \
	html/_sources/tutorial/tutorial04.txt					  \
	html/_sources/tutorial/tutorial05.txt					  \
	html/_sources/tutorial/tutorial06.txt					  \
	html/_sources/tutorial/tutorial07.txt					  \
	html/_sources/tutorial/tutorial08.txt					  \
	html/_sources/tutorial/tutorial09.txt					  \
	html/_sources/tutorial/tutorial10.txt					  \
	html/_sources/type.txt							  \
	html/_static/basic.css							  \
	html/_static/default.css						  \
	html/_static/doctools.js						  \
	html/_static/file.png							  \
	html/_static/jquery.js							  \
	html/_static/minus.png							  \
	html/_static/plus.png							  \
	html/_static/pygments.css						  \
	html/_static/searchtools.js						  \
	html/_static/sidebar.js							  \
	html/_static/underscore.js						  \
	html/characteristic.html						  \
	html/command_version.html						  \
	html/commands.html							  \
	html/commands/cache_limit.html						  \
	html/commands/check.html						  \
	html/commands/clearlock.html						  \
	html/commands/column_create.html					  \
	html/commands/column_list.html						  \
	html/commands/column_remove.html					  \
	html/commands/define_selector.html					  \
	html/commands/defrag.html						  \
	html/commands/delete.html						  \
	html/commands/dump.html							  \
	html/commands/load.html							  \
	html/commands/log_level.html						  \
	html/commands/log_put.html						  \
	html/commands/log_reopen.html						  \
	html/commands/quit.html							  \
	html/commands/select.html						  \
	html/commands/shutdown.html						  \
	html/commands/status.html						  \
	html/commands/suggest.html						  \
	html/commands/table_create.html						  \
	html/commands/table_list.html						  \
	html/commands/table_remove.html						  \
	html/commands/view_add.html						  \
	html/commands_not_implemented/add.html					  \
	html/commands_not_implemented/get.html					  \
	html/commands_not_implemented/set.html					  \
	html/developer.html							  \
	html/developer/com.html							  \
	html/developer/document.html						  \
	html/developer/query.html						  \
	html/developer/test.html						  \
	html/execfile.html							  \
	html/expr.html								  \
	html/functions.html							  \
	html/functions/edit_distance.html					  \
	html/functions/geo_distance.html					  \
	html/functions/geo_in_circle.html					  \
	html/functions/geo_in_rectangle.html					  \
	html/functions/now.html							  \
	html/functions/rand.html						  \
	html/genindex.html							  \
	html/grnslap.html							  \
	html/grntest.html							  \
	html/http.html								  \
	html/index.html								  \
	html/install.html							  \
	html/news.html								  \
	html/objects.inv							  \
	html/process.html							  \
	html/pseudo_column.html							  \
	html/reference.html							  \
	html/search.html							  \
	html/searchindex.js							  \
	html/spec.html								  \
	html/spec/search.html							  \
	html/troubleshooting.html						  \
	html/troubleshooting/different_results_with_the_same_keyword.html	  \
	html/tutorial.html							  \
	html/tutorial/tutorial01.html						  \
	html/tutorial/tutorial02.html						  \
	html/tutorial/tutorial03.html						  \
	html/tutorial/tutorial04.html						  \
	html/tutorial/tutorial05.html						  \
	html/tutorial/tutorial06.html						  \
	html/tutorial/tutorial07.html						  \
	html/tutorial/tutorial08.html						  \
	html/tutorial/tutorial09.html						  \
	html/tutorial/tutorial10.html						  \
	html/type.html

$(html_files): html-build-stamp
html-build-stamp:
	$(MAKE) html
endif
