SUBDIRS =			\
	images			\
	html			\
	munin			\
	init.d			\
	systemd

pkgsysconfdir = $(sysconfdir)/$(PACKAGE)
dist_pkgsysconf_DATA =		\
	groonga.conf

httpdconfdir = $(pkgsysconfdir)/httpd
dist_httpdconf_DATA =		\
	groonga-httpd.conf

EXTRA_DIST =					\
	CMakeLists.txt
