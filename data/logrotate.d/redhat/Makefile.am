
logrotate_conf = groonga-server groonga-httpd

if REDHAT_PLATFORM
logrotatedir = $(sysconfdir)/logrotate.d
dist_logrotate_DATA = $(logrotate_conf)
else
EXTRA_DIST = $(logrotate_conf)
endif
