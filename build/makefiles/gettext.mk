update: $(mo_files)

.po.mo:
	msgfmt -o $@ $<

NULL =

# (cd ../../doc; echo "po_files = \\"; find pot -type f -name '*.pot' | sort | sed -e 's,^pot/,\t,g' -e 's,pot$,po \\,'; echo -n "\t\$(NULL)")
po_files =					\
	characteristic.po			\
	command_version.po			\
	commands.po				\
	commands_not_implemented.po		\
	developer.po				\
	execfile.po				\
	expr.po					\
	functions.po				\
	grnslap.po				\
	grntest.po				\
	http.po					\
	index.po				\
	install.po				\
	limitations.po				\
	news.po					\
	process.po				\
	pseudo_column.po			\
	reference.po				\
	spec.po					\
	troubleshooting.po			\
	tutorial.po				\
	type.po

# (cd ../../doc; echo "mo_files = \\"; find pot -type f -name '*.pot' | sort | sed -e 's,^pot/,\t,g' -e 's,pot$,mo \\,'; echo -n "\t\$(NULL)")
mo_files = \
	characteristic.mo \
	command_version.mo \
	commands.mo \
	commands_not_implemented.mo \
	developer.mo \
	execfile.mo \
	expr.mo \
	functions.mo \
	grnslap.mo \
	grntest.mo \
	http.mo \
	index.mo \
	install.mo \
	limitations.mo \
	news.mo \
	process.mo \
	pseudo_column.mo \
	reference.mo \
	spec.mo \
	troubleshooting.mo \
	tutorial.mo \
	type.mo \
	$(NULL)
