# release: update-latest-release (commit) tag

LOCALES = ja

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = 1.9.6
SUBDIRS =					\
	build					\
	cmake					\
	include					\
	vendor					\
	lib					\
	plugins					\
	src					\
	examples				\
	test					\
	benchmark				\
	packages				\
	data					\
	tools					\
	doc
EXTRA_DIST =					\
	CMakeLists.txt				\
	README.md				\
	base_version				\
	bindings				\
	bundled_lz4_version			\
	bundled_mecab_naist_jdic_version	\
	bundled_mecab_version			\
	bundled_rapidjson_version		\
	bundled_xxhash_version			\
	config.h.cmake.in			\
	gpg_uid					\
	setup.sh				\
	version-gen.sh
pkgdata_DATA = COPYING

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = groonga.pc
if GRN_WITH_APACHE_ARROW
pkgconfig_DATA += groonga-arrow.pc
endif

.PHONY: FORCE

FORCE:

$(srcdir)/version.sh: FORCE
	cd "$(srcdir)" && $(SHELL) version-gen.sh

include $(srcdir)/version.sh

dist-hook:
	echo "$(GRN_VERSION)" > $(distdir)/version
	echo "GRN_VERSION_FULL=$(GRN_VERSION)" > $(distdir)/version.sh
	cd $(distdir) && autoreconf --install --force && find . -name autom4te.cache | xargs rm -fr

benchmark:
	cd test/benchmark && $(MAKE) benchmark

tag:
	cd $(top_srcdir) && git tag v$(VERSION) -a -m 'Groonga $(VERSION)!!!'

echo-version:
	@echo $(VERSION)

update-latest-release:
	cd $(top_srcdir) &&					\
	  $(RUBY) -S rake release:version:update

update-po:
	@for lang in $(LOCALES); do						\
	  (cd $(top_srcdir)/doc/locale/$$lang/LC_MESSAGES && $(MAKE) update)	\
	done

update-document:
	@if test -z "$(GROONGA_ORG_PATH)"; then				\
	  echo "\$$(GROONGA_ORG_PATH) is missing";			\
	  echo "add --with-groonga-org-path in configure";	\
	  exit 1;							\
	fi
	rm -rf tmp-doc
	mkdir tmp-doc
	(cd doc && $(MAKE) clean-html)
	(cd doc && $(MAKE) install docdir=$(abs_builddir)/tmp-doc/install)
	rm -rf $(GROONGA_ORG_PATH)/docs
	mv tmp-doc/install/en/html $(GROONGA_ORG_PATH)/docs
	rm -rf tmp-doc/install/en
	for locale in $$(cd tmp-doc/install/; echo *); do	\
	  dest_base_dir=$(GROONGA_ORG_PATH)/$${locale};		\
	  mkdir -p $${dest_base_dir};				\
	  dest_dir=$${dest_base_dir}/docs;			\
	  rm -rf $${dest_dir};					\
	  mv tmp-doc/install/$${locale}/html $${dest_dir};	\
	done


update-version:
	@if test -z "$(NEW_VERSION)"; then	\
	  echo "\$$(NEW_VERSION) is missing";	\
	  exit 1;				\
	fi
	@echo -n $(NEW_VERSION) > $(srcdir)/base_version

update-files:
	cd doc && $(MAKE) update-files
	cd data/html && $(MAKE) update-files

update-mruby:
	cd vendor/mruby && make update
