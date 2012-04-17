htmldir = $(pkgdatadir)/html

include nobase_html_files.am

EXTRA_DIST =					\
	CMakeLists.txt

update-files:
	cd $(srcdir); ./update-files.sh > nobase_html_files.am
