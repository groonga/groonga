SUBDIRS =					\
	ja

add:
	test -n "$(LOCALE)" ||				\
	  (echo "run with LOCALE=\$${NEW_LOCALE}.";	\
	   echo "e.g.: make add LOCALE=ja";		\
           false)
	mkdir -p $(srcdir)/$(LOCALE)/LC_MESSAGE
	sed -e 's,%LOCALE%,$(LOCALE),g'				\
	  $(srcdir)/$(LOCALE)/template.locale.Makefile.am	\
	    > $(srcdir)/$(LOCALE)/Makefile.am
	sed -e 's,%LOCALE%,$(LOCALE),g'					\
	  $(srcdir)/$(LOCALE)/template.locale.LC_MESSAGES.Makefile.am	\
	    > $(srcdir)/$(LOCALE)/LC_MESSAGES/Makefile.am
	sed -i''					\
	  -e 's,SUBDIRS =,SUBDIRS =\n\t$(LOCALE),'	\
	  $(srcdir)/Makefile.am
	sed -i''						\
	  -e "/doc\/locale\/ja\/Makefile/			\
              a doc\/locale\/$(LOCALE)\/Makefile"		\
	  -e "/doc\/locale\/ja\/LC_MESSAGES\/Makefile/		\
              a doc\/locale\/$(LOCALE)\/LC_MESSAGES\Makefile"	\
	  $(srcdir)/configure.ac

