SUBDIRS =			\
	html			\
	munin			\
	logrotate.d		\
	systemd			\
	scripts			\
	tmpfiles.d

pkgsysconfdir = $(sysconfdir)/$(PACKAGE)
dist_pkgsysconf_DATA =		\
	groonga.conf		\
	synonyms.tsv

httpdconfdir = $(pkgsysconfdir)/httpd
dist_httpdconf_DATA =		\
	groonga-httpd.conf

EXTRA_DIST =					\
	CMakeLists.txt
