include files.am

imagesdir = $(pkgdatadir)/images
images_DATA = $(image_files)

EXTRA_DIST =					\
	$(images_DATA)				\
	update-files.sh

SUFFIXES =
if WITH_INKSCAPE
SUFFIXES += .svg .png
.svg.png:
	$(INKSCAPE) --export-dpi 90 --export-png $@ $<
endif

$(srcdir)/files.am: $(srcdir)/update-files.sh
	$(srcdir)/update-files.sh > $(srcdir)/files.am

update-files:
	rm $(srcdir)/files.am
	$(MAKE) $(srcdir)/files.am
