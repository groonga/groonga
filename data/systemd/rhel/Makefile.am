SUBDIRS = sysconfig

services =					\
	groonga-server-http.service		\
	groonga-server-gqtp.service

if RHEL_PLATFORM
unitdir = /usr/lib/systemd/system
dist_unit_DATA = $(services)
else
EXTRA_DIST = $(services)
endif
