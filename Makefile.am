ACLOCAL_AMFLAGS = $$ACLOCAL_ARGS -I .
AUTOMAKE_OPTIONS = 1.9.7
SUBDIRS = include lib plugins src examples test apt rpm yum data resource doc
#dist_data_DATA =
EXTRA_DIST = bindings version-gen.sh

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = groonga.pc

.PHONY: FORCE

version.sh: FORCE
	@$(SHELL_PATH) $(srcdir)/version-gen.sh

@am__include@ $(srcdir)/version.sh

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
