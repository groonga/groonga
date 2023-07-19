tmpfiles_conf = groonga.conf

if RHEL_PLATFORM
tmpfilesdir = $(sysconfdir)/tmpfiles.d
dist_tmpfiles_DATA = $(tmpfiles_conf)
else
EXTRA_DIST = $(tmpfiles_conf)
endif
