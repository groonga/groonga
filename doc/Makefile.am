SUBDIRS =					\
	locale

docdir = $(pkgdatadir)/doc

include $(abs_top_srcdir)/build/makefiles/sphinx-build.mk

update-examples:
	GROONGA=$(GROONGA) $(srcdir)/source/update_execution_example.py

gettext: sphinx-ensure-updated
	$(SPHINX_BUILD_COMMAND) -b gettext $(ALLSPHINXOPTS) pot

add-locale: gettext
	test -n "$(LOCALE)" ||				\
	  (echo "run with LOCALE=\$${NEW_LOCALE}.";	\
	   echo "e.g.: make add-locale LOCALE=ja";	\
           false)
	mkdir -p $(srcdir)/locale/$(LOCALE)/LC_MESSAGE
	for pot in pot/*.pot; do					\
	  po=`basename $$pot | sed s/pot$$/po/`;			\
	  msginit							\
	    --input "$$pot"						\
	    --output $(srcdir)/locale/$(LOCALE)/LC_MESSAGE/$${po}	\
	    --locale=$(LOCALE);						\
	done

update-po: gettext
	for locale in $(LOCALES); do				\
	  test $${locale} = "en" && continue;				\
	  for po in $(srcdir)/locale/$${locale}/LC_MESSAGES/*.po; do	\
	    pot=pot/`basename $$po | sed s/po$$/pot/`;			\
	    echo -n "updateing $${po}: ";				\
	    msgmerge							\
	      --update							\
	      --sort-by-file						\
	      $${po}							\
	      $${pot};							\
	  done;								\
	done

create-mo:
	for locale in $(LOCALES); do				\
	  test $${locale} = "en" && continue;				\
	  for po in $(srcdir)/locale/$${locale}/LC_MESSAGES/*.po; do	\
	    mo=`basename $${po} | sed s/po$$/mo/`;			\
	    mo=locale/$${locale}/LC_MESSAGES/$${mo};			\
	    echo "creating $${mo}";					\
	    mkdir -p `dirname $${mo}`;					\
	    msgfmt -o $${mo} $${po};					\
	  done;								\
	done
	$(MAKE) clean-doctree

# find pot -type f -name '*.pot' | sort | sed -e 's,^,\t,g'
pot_files =					\
	pot/characteristic.pot			\
	pot/command_version.pot			\
	pot/commands.pot			\
	pot/commands_not_implemented.pot	\
	pot/developer.pot			\
	pot/execfile.pot			\
	pot/expr.pot				\
	pot/functions.pot			\
	pot/grnslap.pot				\
	pot/grntest.pot				\
	pot/http.pot				\
	pot/index.pot				\
	pot/install.pot				\
	pot/limitations.pot			\
	pot/news.pot				\
	pot/process.pot				\
	pot/pseudo_column.pot			\
	pot/reference.pot			\
	pot/spec.pot				\
	pot/troubleshooting.pot			\
	pot/tutorial.pot			\
	pot/type.pot

if ENABLE_DOCUMENT
dist-hook:
	@touch $(distdir)/man-build-stamp
	@touch $(distdir)/html-build-stamp

dist_man1_MANS =				\
	man/groonga.1

$(dist_man1_MANS): man-build-stamp
man-build-stamp:
	$(MAKE) man

nobase_dist_doc_DATA =				\
	$(source_files)				\
	$(html_files)

# find html -type f | sort | sed -e 's,^,\t,g'
html_files =									  \
	html/.buildinfo								  \
	html/_sources/characteristic.txt					  \
	html/_sources/command_version.txt					  \
	html/_sources/commands.txt						  \
	html/_sources/commands/cache_limit.txt					  \
	html/_sources/commands/check.txt					  \
	html/_sources/commands/clearlock.txt					  \
	html/_sources/commands/column_create.txt				  \
	html/_sources/commands/column_list.txt					  \
	html/_sources/commands/column_remove.txt				  \
	html/_sources/commands/define_selector.txt				  \
	html/_sources/commands/defrag.txt					  \
	html/_sources/commands/delete.txt					  \
	html/_sources/commands/dump.txt						  \
	html/_sources/commands/load.txt						  \
	html/_sources/commands/log_level.txt					  \
	html/_sources/commands/log_put.txt					  \
	html/_sources/commands/log_reopen.txt					  \
	html/_sources/commands/quit.txt						  \
	html/_sources/commands/select.txt					  \
	html/_sources/commands/shutdown.txt					  \
	html/_sources/commands/status.txt					  \
	html/_sources/commands/suggest.txt					  \
	html/_sources/commands/table_create.txt					  \
	html/_sources/commands/table_list.txt					  \
	html/_sources/commands/table_remove.txt					  \
	html/_sources/commands/view_add.txt					  \
	html/_sources/commands_not_implemented/add.txt				  \
	html/_sources/commands_not_implemented/get.txt				  \
	html/_sources/commands_not_implemented/set.txt				  \
	html/_sources/developer.txt						  \
	html/_sources/developer/com.txt						  \
	html/_sources/developer/document.txt					  \
	html/_sources/developer/query.txt					  \
	html/_sources/developer/test.txt					  \
	html/_sources/execfile.txt						  \
	html/_sources/expr.txt							  \
	html/_sources/functions.txt						  \
	html/_sources/functions/edit_distance.txt				  \
	html/_sources/functions/geo_distance.txt				  \
	html/_sources/functions/geo_in_circle.txt				  \
	html/_sources/functions/geo_in_rectangle.txt				  \
	html/_sources/functions/now.txt						  \
	html/_sources/functions/rand.txt					  \
	html/_sources/grnslap.txt						  \
	html/_sources/grntest.txt						  \
	html/_sources/http.txt							  \
	html/_sources/index.txt							  \
	html/_sources/install.txt						  \
	html/_sources/limitations.txt						  \
	html/_sources/news.txt							  \
	html/_sources/process.txt						  \
	html/_sources/pseudo_column.txt						  \
	html/_sources/reference.txt						  \
	html/_sources/spec.txt							  \
	html/_sources/spec/search.txt						  \
	html/_sources/troubleshooting.txt					  \
	html/_sources/troubleshooting/different_results_with_the_same_keyword.txt \
	html/_sources/tutorial.txt						  \
	html/_sources/tutorial/tutorial01.txt					  \
	html/_sources/tutorial/tutorial02.txt					  \
	html/_sources/tutorial/tutorial03.txt					  \
	html/_sources/tutorial/tutorial04.txt					  \
	html/_sources/tutorial/tutorial05.txt					  \
	html/_sources/tutorial/tutorial06.txt					  \
	html/_sources/tutorial/tutorial07.txt					  \
	html/_sources/tutorial/tutorial08.txt					  \
	html/_sources/tutorial/tutorial09.txt					  \
	html/_sources/tutorial/tutorial10.txt					  \
	html/_sources/type.txt							  \
	html/_static/basic.css							  \
	html/_static/default.css						  \
	html/_static/doctools.js						  \
	html/_static/file.png							  \
	html/_static/jquery.js							  \
	html/_static/minus.png							  \
	html/_static/plus.png							  \
	html/_static/pygments.css						  \
	html/_static/searchtools.js						  \
	html/_static/sidebar.js							  \
	html/_static/underscore.js						  \
	html/characteristic.html						  \
	html/command_version.html						  \
	html/commands.html							  \
	html/commands/cache_limit.html						  \
	html/commands/check.html						  \
	html/commands/clearlock.html						  \
	html/commands/column_create.html					  \
	html/commands/column_list.html						  \
	html/commands/column_remove.html					  \
	html/commands/define_selector.html					  \
	html/commands/defrag.html						  \
	html/commands/delete.html						  \
	html/commands/dump.html							  \
	html/commands/load.html							  \
	html/commands/log_level.html						  \
	html/commands/log_put.html						  \
	html/commands/log_reopen.html						  \
	html/commands/quit.html							  \
	html/commands/select.html						  \
	html/commands/shutdown.html						  \
	html/commands/status.html						  \
	html/commands/suggest.html						  \
	html/commands/table_create.html						  \
	html/commands/table_list.html						  \
	html/commands/table_remove.html						  \
	html/commands/view_add.html						  \
	html/commands_not_implemented/add.html					  \
	html/commands_not_implemented/get.html					  \
	html/commands_not_implemented/set.html					  \
	html/developer.html							  \
	html/developer/com.html							  \
	html/developer/document.html						  \
	html/developer/query.html						  \
	html/developer/test.html						  \
	html/execfile.html							  \
	html/expr.html								  \
	html/functions.html							  \
	html/functions/edit_distance.html					  \
	html/functions/geo_distance.html					  \
	html/functions/geo_in_circle.html					  \
	html/functions/geo_in_rectangle.html					  \
	html/functions/now.html							  \
	html/functions/rand.html						  \
	html/genindex.html							  \
	html/grnslap.html							  \
	html/grntest.html							  \
	html/http.html								  \
	html/index.html								  \
	html/install.html							  \
	html/limitations.html							  \
	html/news.html								  \
	html/objects.inv							  \
	html/process.html							  \
	html/pseudo_column.html							  \
	html/reference.html							  \
	html/search.html							  \
	html/searchindex.js							  \
	html/spec.html								  \
	html/spec/search.html							  \
	html/troubleshooting.html						  \
	html/troubleshooting/different_results_with_the_same_keyword.html	  \
	html/tutorial.html							  \
	html/tutorial/tutorial01.html						  \
	html/tutorial/tutorial02.html						  \
	html/tutorial/tutorial03.html						  \
	html/tutorial/tutorial04.html						  \
	html/tutorial/tutorial05.html						  \
	html/tutorial/tutorial06.html						  \
	html/tutorial/tutorial07.html						  \
	html/tutorial/tutorial08.html						  \
	html/tutorial/tutorial09.html						  \
	html/tutorial/tutorial10.html						  \
	html/type.html

$(html_files): html-build-stamp
html-build-stamp:
	$(MAKE) html
endif
