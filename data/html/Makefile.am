htmldir = $(pkgdatadir)/html

include html_files.am

EXTRA_DIST =					\
	CMakeLists.txt

update-files:
	cd $(srcdir); ./update-files.sh > html_files.am
