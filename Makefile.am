AUTOMAKE_OPTIONS = 1.9.7
SUBDIRS = lib modules src examples test apt yum data resource doc
#dist_data_DATA =
EXTRA_DIST = bindings rpm version-gen.sh
pkginclude_HEADERS = groonga.h

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
	cp -p $(builddir)/groonga.h dist/include/
	nkf -Wsc $(builddir)/doc/ja/README > dist/doc/README.txt
	makensis setup.nsi
