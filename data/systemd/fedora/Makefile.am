SUBDIRS = sysconfig

services =					\
	groonga.service				\
	groonga-httpd.service

if FEDORA_PLATFORM
unitdir = /usr/lib/systemd/system
dist_unit_DATA = $(services)
else
EXTRA_DIST = $(services)
endif
