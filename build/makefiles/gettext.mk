include $(abs_top_srcdir)/build/makefiles/gettext-files.mk
include $(abs_top_srcdir)/build/makefiles/sphinx-build.mk

.PHONY: gettext update build

all: build

.SUFFIXES: .pot .po .mo
.pot.po:
	msgmerge --quiet --update --sort-by-file $@ $<
.po.mo:
	msgfmt -o $@ $<

update: pot-build-stamp $(po_files)
build: $(mo_files)

html: build
pdf: build

gettext: sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND) -d doctrees -b gettext $(ALLSPHINXOPTS) .

pot-build-stamp: $(source_files)
	$(MAKE) gettext
	@touch $@
