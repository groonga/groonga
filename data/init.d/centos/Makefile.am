SUBDIRS = sysconfig

scripts = groonga-server-http groonga-server-gqtp groonga-httpd

if CENTOS_PLATFORM
initdir = $(sysconfdir)/init.d
dist_init_SCRIPTS = $(scripts)
else
EXTRA_DIST = $(scripts)
endif
