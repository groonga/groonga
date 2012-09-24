data = groonga-server-gqtp groonga-server-http groonga-httpd

if REDHAT_PLATFORM
sysconfigdir = $(sysconfdir)/sysconfig
dist_sysconfig_DATA = $(data)
else
EXTRA_DIST = $(data)
endif
