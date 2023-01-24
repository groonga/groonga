SUBDIRS = \
	en \
	ja

am__extra_recursive_targets = 	\
	text-recursive		\
	markdown-recursive

add:
	test -n "$(LOCALE)" ||				\
	  (echo "run with LOCALE=\$${NEW_LOCALE}.";	\
	   echo "e.g.: make add LOCALE=ja";		\
           false)
	mkdir -p $(srcdir)/$(LOCALE)/LC_MESSAGES
	sed -e 's,%LOCALE%,$(LOCALE),g'		\
	  $(srcdir)/template.locale.Makefile.am	\
	    > $(srcdir)/$(LOCALE)/Makefile.am
	sed -e 's,%LOCALE%,$(LOCALE),g'				\
	  $(srcdir)/template.locale.LC_MESSAGES.Makefile.am	\
	    > $(srcdir)/$(LOCALE)/LC_MESSAGES/Makefile.am
	sed -i''					\
	  -e 's,^SUBDIRS =,SUBDIRS = \\\n\t$(LOCALE),'	\
	  $(srcdir)/Makefile.am
	sed -i''							\
	  -e "\,doc/locale/Makefile,					\
              a \ \ doc\/locale\/$(LOCALE)\/Makefile"			\
	  -e "\,doc/locale/Makefile,					\
              a \ \ doc\/locale\/$(LOCALE)\/LC_MESSAGES\/Makefile"	\
	  $(top_srcdir)/configure.ac

clean-html init update-po:
	for dir in $(SUBDIRS); do		\
	  (cd $${dir} && $(MAKE) $@);		\
	done

text: text-recursive
text-am:
markdown: markdown-recursive
markdown-am:
