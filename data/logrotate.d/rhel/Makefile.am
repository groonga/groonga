logrotate_conf = groonga-server-gqtp groonga-server-http

if RHEL_PLATFORM
logrotatedir = $(sysconfdir)/logrotate.d
dist_logrotate_DATA = $(logrotate_conf)
else
EXTRA_DIST = $(logrotate_conf)
endif
