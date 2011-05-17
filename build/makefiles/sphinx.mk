include $(abs_topsrcdir)/build/makefiles/shpinx-build.mk

# You can set these variables from the command line.
DOCTREES_BASE = doctrees
DOC_LANGUAGES = en ja

# Internal variables.
PAPEROPT_a4     = -D latex_paper_size=a4
PAPEROPT_letter = -D latex_paper_size=letter
ALLSPHINXOPTS   = $(PAPEROPT_$(PAPER)) $(SPHINXOPTS) $(abs_top_srcdir)/doc/source

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
	@echo "  textile   to make Textile files"
	@echo "  changes   to make an overview of all changed/added/deprecated items"
	@echo "  linkcheck to check all external links for integrity"
	@echo "  doctest   to run all doctests embedded in the documentation (if enabled)"

clean-doctree:
	-rm -rf $(DOCTREES_BASE)

clean-local: clean-doctree
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
	-rm -rf $(BUILDDIR)/textile
	-rm -rf $(BUILDDIR)/changes
	-rm -rf $(BUILDDIR)/linkcheck
	-rm -rf $(BUILDDIR)/doctest
	-rm -rf $(BUILDDIR)/pdf

man: sphinx-ensure-updated man/groonga.1

man/groonga.1: $(source_files)
	$(SPHINX_BUILD_COMMAND)				\
	  -Dlanguage=$(DOC_LANGUAGE)			\
	  -d $(DOCTREES_BASE)/man	\
	  -b man					\
	  $(ALLSPHINXOPTS)				\
	  man

html: sphinx-ensure-updated html/index.html

html/index.html: $(source_files)
	$(SPHINX_BUILD_COMMAND)				\
	  -Dlanguage=$(DOC_LANGUAGE)			\
	  -d $(DOCTREES_BASE)html	\
	  -b html					\
	  $(ALLSPHINXOPTS)				\
	  html

dirhtml: sphinx-ensure-updated dirhtml/index.html

dirhtml/index.html: $(source_files)
	$(SPHINX_BUILD_COMMAND)				\
	  -Dlanguage=$(DOC_LANGUAGE)			\
	  -d $(DOCTREES_BASE)/dirhtml			\
	  -b dirhtml					\
	  $(ALLSPHINXOPTS)				\
          dirhtml

pickle: sphinx-ensure-updated pickle/index.fpickle

pickle/index.fpickle: $(source_files)
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(DOC_LANGUAGE)		\
	  -d $(DOCTREES_BASE)/pickle		\
	  -b pickle				\
	  $(ALLSPHINXOPTS)			\
	  pickle

json: sphinx-ensure-updated json/index.fjson

json/index.fjson: $(source_files)
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(DOC_LANGUAGE)		\
	  -d $(DOCTREES_BASE)/json		\
	  -b json				\
	  $(ALLSPHINXOPTS)			\
	  json

htmlhelp: sphinx-ensure-updated htmlhelp/index.html

htmlhelp/index.html: $(source_files)
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(DOC_LANGUAGE)		\
	  -d $(DOCTREES_BASE)/htmlhelp		\
	  -b htmlhelp				\
	  $(ALLSPHINXOPTS)			\
	  htmlhelp

qthelp: sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(DOC_LANGUAGE)		\
	  -d $(DOCTREES_BASE)/qthelp		\
	  -b qthelp				\
	  $(ALLSPHINXOPTS)			\
	  qthelp
	@echo
	@echo "Build finished; now you can run 'qcollectiongenerator' with the" \
	      ".qhcp project file in qthelp/*/, like this:"
	@echo "# qcollectiongenerator qthelp/groonga.qhcp"
	@echo "To view the help file:"
	@echo "# assistant -collectionFile qthelp/groonga.qhc"

latex: sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(DOC_LANGUAGE)		\
	  -d $(DOCTREES_BASE)/latex		\
	  -b latex				\
	  $(ALLSPHINXOPTS)			\
	  latex
	@echo
	@echo "Build finished; the LaTeX files are in latex/*/."
	@echo "Run \`make all-pdf' or \`make all-ps' in that directory to" \
	      "run these through (pdf)latex."

rdoc: sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(DOC_LANGUAGE)		\
	  -d $(DOCTREES_BASE)/rdoc		\
	  -b rdoc				\
	  $(ALLSPHINXOPTS)			\
	  rdoc

textile: sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(DOC_LANGUAGE)		\
	  -d $(DOCTREES_BASE)/textile		\
	  -b textile				\
	  $(ALLSPHINXOPTS)			\
	  textile

changes: sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(DOC_LANGUAGE)		\
	  -d $(DOCTREES_BASE)/changes		\
	  -b changes				\
	  $(ALLSPHINXOPTS)			\
	  changes

linkcheck: sphinx-ensure-updated linkcheck/output.txt

linkcheck/output.txt: $(source_files)
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(DOC_LANGUAGE)		\
	  -d $(DOCTREES_BASE)/linkcheck		\
	  -b linkcheck				\
	  $(ALLSPHINXOPTS)			\
	  linkcheck

doctest: sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(DOC_LANGUAGE)		\
	  -d $(DOCTREES_BASE)/doctest		\
	  -b doctest				\
	  $(ALLSPHINXOPTS)			\
	  doctest

pdf: sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND)			\
	  -Dlanguage=$(DOC_LANGUAGE)		\
	  -d $(DOCTREES_BASE)/pdf		\
	  -b pdf				\
	  $(ALLSPHINXOPTS)			\
	  pdf

if ENABLE_DOCUMENT
dist-hook:
	@touch $(distdir)/man-build-stamp
	@touch $(distdir)/html-build-stamp

dist_man1_MANS =				\
	man/groonga.1

$(dist_man1_MANS): man-build-stamp
man-build-stamp:
	$(MAKE) man

nobase_dist_doc_DATA =				\
	$(source_files)				\
	$(html_files)

$(html_files): html-build-stamp
html-build-stamp:
	$(MAKE) html
endif
