NGINX_DIR = nginx-$(NGINX_VERSION)
GROONGALOG_DIR = groonga-log/lib/

SUBDIRS =					\
	lz4					\
	onigmo					\
	mecab					\
	message_pack				\
	mruby

EXTRA_DIST =					\
	$(NGINX_DIR)				\
	CMakeLists.txt				\
	plugins/CMakeLists.txt			\
	download_lz4.rb				\
	download_message_pack.rb		\
	download_mecab.rb		\
	groonga-log

dist-hook:
	rm -rf $(distdir)/$(NGINX_DIR)/objs/
	GIT_DIR=$(srcdir)/mruby-source/.git git archive --format=tar HEAD | \
	  tar xf - -C $(distdir)/mruby-source
	$(MKDIR_P) $(distdir)/onigmo-source
	GIT_DIR=$(srcdir)/onigmo-source/.git git archive --format=tar HEAD | \
	  tar xf - -C $(distdir)/onigmo-source
	cd $(distdir)/onigmo-source && autoreconf --install --force
	cd $(distdir)/onigmo-source && sleep 1 && touch config.h.in
	$(MKDIR_P) $(distdir)/ngx_mruby-source
	GIT_DIR=$(srcdir)/ngx_mruby-source/.git git archive --format=tar HEAD | \
	  tar xf - -C $(distdir)/ngx_mruby-source

install-data-hook:
  RUBY_SCRIPT_FILES =       \
    $(GROONGALOG_DIR)/groonga-log/parser.rb       \
    $(GROONGALOG_DIR)/groonga-log/statistic.rb        \
    $(GROONGALOG_DIR)/groonga-log/version.rb

  ruby_scripts_groonga_logdir = $(ruby_scriptsdir)/groonga-log
  ruby_scripts_groonga_log_DATA = $(RUBY_SCRIPT_FILES)
  ruby_scripts_DATA = $(GROONGALOG_DIR)/groonga-log.rb

