NGINX_DIR = nginx-$(NGINX_VERSION)

SUBDIRS =					\
	onigmo					\
	mruby

EXTRA_DIST =					\
	$(NGINX_DIR)				\
	mruby					\
	CMakeLists.txt				\
	plugins/CMakeLists.txt

dist-hook:
	GIT_DIR=$(srcdir)/mruby-source/.git git archive --format=tar HEAD | \
	  tar xf - -C $(distdir)/mruby-source
	GIT_DIR=$(srcdir)/onigmo-source/.git git archive --format=tar HEAD | \
	  tar xf - -C $(distdir)/onigmo-source
