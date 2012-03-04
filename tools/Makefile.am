install_ruby_scripts =				\
	groonga-query-log-analyzer

noinstall_ruby_scripts =			\
	groonga-memory-leak-checker.rb		\
	prepare-sphinx-html.rb

EXTRA_DIST =					\
	$(install_ruby_scripts)			\
	$(noinstall_ruby_scripts)

if WITH_RUBY
install-exec-hook:
	for script in $(install_ruby_scripts); do		\
	  sed -e '1s,#!.*,#!$(RUBY),'				\
	    $${script} > $(DESTDIR)$(bindir)/$${script};	\
	  chmod +x $(DESTDIR)$(bindir)/$${script};		\
	done
endif
