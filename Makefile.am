SUBDIRS = lib src examples test
#dist_data_DATA =
EXTRA_DIST = bindings resource doc/ja
pkginclude_HEADERS = groonga.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = groonga.pc

LCOV_TMP = lcov_tmp
LCOV_HTML = lcov_html
LCOV_LCOV = @LCOV@
LCOV_GENHTML = @GENHTML@

dist-hook:
	rm -rf `find $(distdir) -name '*.loT'`
	rm -rf `find $(distdir) -name '*.cache'`
	rm -rf `find $(distdir) -name '*.bak'`
	rm -rf `find $(distdir) -name '*.orig'`
	rm -rf $(distdir)/doc/ja/_sources
	rm -rf $(distdir)/doc/ja/build
	rm -rf $(distdir)/doc/ja/source

install-data-local:
	@$(NORMAL_INSTALL)
	test -z "$(DESTDIR)$(pkgdatadir)" || $(MKDIR_P) "$(DESTDIR)$(pkgdatadir)"
	cp -R resource/* $(DESTDIR)$(pkgdatadir)

uninstall-local:
	@$(NORMAL_UNINSTALL)
	rm -rf $(DESTDIR)$(pkgdatadir)

ruby-bindings:
	cd bindings/ruby && \
	ruby extconf.rb && \
	make clean && make

install-ruby-bindings:
	cd bindings/ruby && \
	make install

test-ruby-bindings:
	cd bindings/ruby/test && \
	ruby all_test.rbx

valgrind-ruby-bindings:
	cd bindings/ruby/test && \
	valgrind --leak-check=full --show-reachable=yes ruby all_test.rbx

benchmark:
	cd test/benchmark && $(MAKE) $(AM_MAKEFLAGS) benchmark
