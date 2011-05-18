include $(abs_top_srcdir)/build/makefiles/gettext-files.mk

.PHONY: update

all: update

.SUFFIXES: .po .mo
.po.mo:
	msgfmt -o $@ $<

update: $(mo_files)

html: update
pdf: update
