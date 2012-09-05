
scripts = groonga-server groonga-httpd

if REDHAT_PLATFORM
logrotatedir = $(sysconfdir)/logrotate.d
dist_logrotate_SCRIPTS = $(scripts)
else
EXTRA_DIST = $(scripts)
endif
