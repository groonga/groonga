AUTOMAKE_OPTIONS = 1.9.7
SUBDIRS = lib src examples test
#dist_data_DATA =
EXTRA_DIST = bindings resource doc/ja version-gen.sh
pkginclude_HEADERS = groonga.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = groonga.pc

.PHONY: FORCE

version.sh: FORCE
	@$(SHELL_PATH) ./version-gen.sh
-include version.sh

dist-hook:
	rm -rf `find $(distdir) -name '*.loT'`
	rm -rf `find $(distdir) -name '*.cache'`
	rm -rf `find $(distdir) -name '*.bak'`
	rm -rf `find $(distdir) -name '*.orig'`
	find $(distdir)/doc/ja -type f | grep -Ev "/(README|.+\.(rd|js|html|css|png))$$" | xargs rm -f
	rm -rf $(distdir)/doc/ja/_sources
	rm -rf $(distdir)/doc/ja/build
	rm -rf $(distdir)/doc/ja/source
	echo "$(GROONGA_VERSION)" > $(distdir)/version

install-data-local:
	@$(NORMAL_INSTALL)
	test -z "$(DESTDIR)$(pkgdatadir)" || $(MKDIR_P) "$(DESTDIR)$(pkgdatadir)"
	cp -R resource/* $(DESTDIR)$(pkgdatadir)

uninstall-local:
	@$(NORMAL_UNINSTALL)
	rm -rf $(DESTDIR)$(pkgdatadir)

benchmark:
	cd test/benchmark && $(MAKE) $(AM_MAKEFLAGS) benchmark
