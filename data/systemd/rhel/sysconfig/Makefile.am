data =						\
	groonga-httpd				\
	groonga-server-http			\
	groonga-server-gqtp

if RHEL_PLATFORM
sysconfigdir = $(sysconfdir)/sysconfig
dist_sysconfig_DATA = $(data)
else
EXTRA_DIST = $(data)
endif
