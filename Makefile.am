# release: update-latest-release (commit) update-changelog (commit) tag

ACLOCAL_AMFLAGS = $$ACLOCAL_ARGS -I .
AUTOMAKE_OPTIONS = 1.9.6
SUBDIRS =					\
	build					\
	include					\
	lib					\
	plugins					\
	src					\
	examples				\
	test					\
	apt					\
	rpm					\
	yum					\
	data					\
	tools					\
	doc
#dist_data_DATA =
EXTRA_DIST = bindings version-gen.sh

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = groonga.pc

.PHONY: FORCE

version.sh: FORCE
	@$(SHELL_PATH) $(srcdir)/version-gen.sh

include $(srcdir)/version.sh

dist-hook:
	echo "$(GROONGA_VERSION)" > $(distdir)/version

benchmark:
	cd test/benchmark && $(MAKE) $(AM_MAKEFLAGS) benchmark

win32-installer: all
	rm -rf dist
	$(MKDIR_P) dist/bin/ dist/include/ dist/lib/ dist/doc/
	cp -p $(builddir)/src/.libs/*.exe dist/bin/
	cp -p $(builddir)/lib/.libs/*.dll dist/bin/
	cp -p $(builddir)/lib/.libs/*.dll.a dist/lib/
	cp -p $(builddir)/lib/.libs/libgroonga.dll.a dist/lib/groonga.lib
	cp -p $(builddir)/include/groonga.h dist/include/
	cp -rp $(builddir)/include/groonga/ dist/include/
	nkf -Wsc $(builddir)/doc/ja/README > dist/doc/README.txt
	makensis setup.nsi

update-changelog:
	cd $(top_srcdir) && release/gitlog2changelog.py

tag:
	cd $(top_srcdir) && git tag v$(VERSION) -a -m 'groonga $(VERSION)!!!'

update-latest-release:
	@if test -z "$(OLD_RELEASE)"; then	\
	  echo "\$$(OLD_RELEASE) is missing";	\
	  exit 1;				\
	fi
	@if test -z "$(OLD_RELEASE_DATE)"; then		\
	  echo "\$$(OLD_RELEASE_DATE) is missing";	\
	  exit 1;					\
	fi
	@if test -z "$(NEW_RELEASE_DATE)"; then		\
	  echo "\$$(NEW_RELEASE_DATE) is missing";	\
	  exit 1;					\
	fi
	cd $(top_srcdir) && test -d "misc" ||					\
	  svn export --force							\
	  https://cutter.svn.sourceforge.net/svnroot/cutter/cutter/trunk/misc
	cd $(top_srcdir) &&					\
	  misc/update-latest-release.rb				\
	  $(PACKAGE) $(OLD_RELEASE) $(OLD_RELEASE_DATE)		\
	  $(VERSION) $(NEW_RELEASE_DATE)			\
	  rpm/fedora/groonga.spec.in				\
	  rpm/centos/groonga.spec.in				\
	  debian/changelog
