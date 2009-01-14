SUBDIRS = lib src
DISTONLY_SUBDIRS = bindings doc util vcc
pkginclude_HEADERS = groonga.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = groonga.pc

LCOV_TMP = lcov_tmp
LCOV_HTML = lcov_html
LCOV_LCOV = @LCOV@
LCOV_GENHTML = @GENHTML@

dist-hook:
# remove vcc
	rm -rf $(distdir)/vcc
# copy dist-only files
	for subdir in $(DISTONLY_SUBDIRS); do \
		cp -rp $$subdir $(distdir)/$$subdir; \
	done
# for mysql udf
	cd $(distdir)/bindings/mysql/udf/; ./autogen.sh; ./configure ; make distclean
	rm -rf `find $(distdir) -name autogen.sh`
# for remove svn directories
	rm -rf `find $(distdir) -name .svn`
# misc
	rm -rf `find $(distdir) -name '*.loT'`
	rm -rf `find $(distdir) -name '*.cache'`
	rm -rf `find $(distdir) -name '*.bak'`
	rm -rf `find $(distdir) -name '*.orig'`

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
