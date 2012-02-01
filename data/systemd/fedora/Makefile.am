SUBDIRS = sysconfig

services = groonga.service

if FEDORA_PLATFORM
unitdir = /lib/systemd/system
dist_unit_DATA = $(services)
else
EXTRA_DIST = $(services)
endif
