generated_images =			\
	geo-encode-leading-2bit.png	\
	geo-encode-leading-4bit.png	\
	geo-points-distance.png		\
	geo-points-in-circle.png	\
	geo-points-in-rectangle.png	\
	geo-points-sort.png		\
	geo-points.png			\
	geo-search-in-circle.png	\
	geo-search-in-rectangle.png

.SUFFIXES: .svg .png
.svg.png:
	inkscape --export-dpi 90 --export-background white --export-png $@ $<

update-images: $(generated_images)
