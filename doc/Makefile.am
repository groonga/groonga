SUBDIRS =					\
	locale

all:

include $(top_srcdir)/build/makefiles/sphinx-build.am

update-examples:
	GROONGA=$(GROONGA) $(srcdir)/source/update_execution_example.py

man pdf clean-html:
	for dir in $(SUBDIRS); do		\
	  (cd $${dir} && $(MAKE) $@);		\
	done

EXTRA_DIST =					\
	$(relative_theme_files)

if ENABLE_DOCUMENT
dist-hook: html man
	@touch $(distdir)/build-stamp
nobase_dist_doc_DATA =				\
	$(relative_source_files)
else
dist-hook:
	@echo "must specify --enable-document for 'make dist'"
	@false
endif

# (echo "relative_source_files = \\"; find source -type f -not -name '*.pyc' | sort | sed -e 's,^,\t,' -e 's,$, \\,'; echo "\t\$(NULL)")
relative_source_files = \
	source/__init__.py \
	source/characteristic.txt \
	source/command_version.txt \
	source/commands.txt \
	source/commands/cache_limit.txt \
	source/commands/check.txt \
	source/commands/clearlock.txt \
	source/commands/column_create.txt \
	source/commands/column_list.txt \
	source/commands/column_remove.txt \
	source/commands/define_selector.txt \
	source/commands/defrag.txt \
	source/commands/delete.txt \
	source/commands/dump.txt \
	source/commands/load.txt \
	source/commands/log_level.txt \
	source/commands/log_put.txt \
	source/commands/log_reopen.txt \
	source/commands/quit.txt \
	source/commands/select.txt \
	source/commands/shutdown.txt \
	source/commands/status.txt \
	source/commands/suggest.txt \
	source/commands/table_create.txt \
	source/commands/table_list.txt \
	source/commands/table_remove.txt \
	source/commands/view_add.txt \
	source/commands_not_implemented/add.txt \
	source/commands_not_implemented/get.txt \
	source/commands_not_implemented/set.txt \
	source/community.txt \
	source/conf.py \
	source/contribution.txt \
	source/contribution/development.txt \
	source/contribution/development/com.txt \
	source/contribution/development/document.txt \
	source/contribution/development/query.txt \
	source/contribution/development/test.txt \
	source/contribution/documentation.txt \
	source/contribution/documentation/c-api.txt \
	source/contribution/documentation/i18n.txt \
	source/contribution/report.txt \
	source/example/tutorial01-1.log \
	source/example/tutorial01-10.log \
	source/example/tutorial01-11.log \
	source/example/tutorial01-12.log \
	source/example/tutorial01-13.log \
	source/example/tutorial01-14.log \
	source/example/tutorial01-15.log \
	source/example/tutorial01-16.log \
	source/example/tutorial01-17.log \
	source/example/tutorial01-2.log \
	source/example/tutorial01-3.log \
	source/example/tutorial01-4.log \
	source/example/tutorial01-5.log \
	source/example/tutorial01-6.log \
	source/example/tutorial01-7.log \
	source/example/tutorial01-8.log \
	source/example/tutorial01-9.log \
	source/example/tutorial02-1.log \
	source/example/tutorial02-2.log \
	source/example/tutorial02-3.log \
	source/example/tutorial03-1.log \
	source/example/tutorial03-2.log \
	source/example/tutorial03-3.log \
	source/example/tutorial04-1.log \
	source/example/tutorial04-2.log \
	source/example/tutorial04-3.log \
	source/example/tutorial04-4.log \
	source/example/tutorial04-5.log \
	source/example/tutorial04-6.log \
	source/example/tutorial04-7.log \
	source/example/tutorial05-1.log \
	source/example/tutorial05-2.log \
	source/example/tutorial05-3.log \
	source/example/tutorial05-4.log \
	source/example/tutorial05-5.log \
	source/example/tutorial05-6.log \
	source/example/tutorial06-1.log \
	source/example/tutorial06-2.log \
	source/example/tutorial06-3.log \
	source/example/tutorial06-4.log \
	source/example/tutorial06-5.log \
	source/example/tutorial06-6.log \
	source/example/tutorial06-7.log \
	source/example/tutorial07-1.log \
	source/example/tutorial07-2.log \
	source/example/tutorial07-3.log \
	source/example/tutorial07-4.log \
	source/example/tutorial08-1.log \
	source/example/tutorial08-2.log \
	source/example/tutorial10-1.log \
	source/example/tutorial10-10.log \
	source/example/tutorial10-2.log \
	source/example/tutorial10-3.log \
	source/example/tutorial10-4.log \
	source/example/tutorial10-5.log \
	source/example/tutorial10-6.log \
	source/example/tutorial10-7.log \
	source/example/tutorial10-8.log \
	source/example/tutorial10-9.log \
	source/executables.txt \
	source/executables/grnslap.txt \
	source/executables/grntest.txt \
	source/executables/groonga.txt \
	source/executables/http.txt \
	source/expr.txt \
	source/functions.txt \
	source/functions/edit_distance.txt \
	source/functions/geo_distance.txt \
	source/functions/geo_in_circle.txt \
	source/functions/geo_in_rectangle.txt \
	source/functions/now.txt \
	source/functions/rand.txt \
	source/index.txt \
	source/install.txt \
	source/limitations.txt \
	source/log.txt \
	source/news.txt \
	source/news/0.x.txt \
	source/news/1.0.x.txt \
	source/news/1.1.x.txt \
	source/pseudo_column.txt \
	source/rdoc.py \
	source/reference.txt \
	source/spec.txt \
	source/spec/search.txt \
	source/suggest.txt \
	source/suggest/introduction.txt \
	source/textile.py \
	source/troubleshooting.txt \
	source/troubleshooting/different_results_with_the_same_keyword.txt \
	source/tutorial.txt \
	source/tutorial/tutorial01.txt \
	source/tutorial/tutorial02.txt \
	source/tutorial/tutorial03.txt \
	source/tutorial/tutorial04.txt \
	source/tutorial/tutorial05.txt \
	source/tutorial/tutorial06.txt \
	source/tutorial/tutorial07.txt \
	source/tutorial/tutorial08.txt \
	source/tutorial/tutorial09.txt \
	source/tutorial/tutorial10.txt \
	source/type.txt \
	source/update_execution_example.py \
	$(NULL)

# (echo "relative_theme_files = \\"; find themes -type f | sort | sed -e 's,^,\t,' -e 's,$, \\,'; echo "\t\$(NULL)")
relative_theme_files = \
	themes/groonga/layout.html \
	themes/groonga/static/favicon.ico \
	themes/groonga/static/footer-bar.png \
	themes/groonga/static/groonga.css_t \
	themes/groonga/static/header-background.png \
	themes/groonga/static/jp.png \
	themes/groonga/static/logo.png \
	themes/groonga/static/menu-header.png \
	themes/groonga/static/navigation-bar.png \
	themes/groonga/static/us.png \
	themes/groonga/theme.conf \
	$(NULL)
