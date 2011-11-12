SUBDIRS = sysconfig

scripts = groonga

if REDHAT_PLATFORM
initdir = $(pkgsysconfdir)/init.d/redhat
dist_init_SCRIPTS = $(scripts)
else
EXTRA_DIST = $(scripts)
endif
