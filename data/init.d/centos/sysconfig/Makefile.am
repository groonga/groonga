data = groonga-server-gqtp groonga-server-http groonga-httpd

if CENTOS_INIT_PLATFORM
sysconfigdir = $(sysconfdir)/sysconfig
dist_sysconfig_DATA = $(data)
else
EXTRA_DIST = $(data)
endif
