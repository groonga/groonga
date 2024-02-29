# Copyright(C) 2017  Brazil
# Copyright(C) 2019-2023  Sutou Kouhei <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

SUBDIRS =					\
	lz4					\
	onigmo					\
	mecab					\
	message_pack				\
	mruby					\
	groonga-log

EXTRA_DIST =					\
	CMakeLists.txt				\
	plugins/CMakeLists.txt			\
	download_groonga_normalizer_mysql.rb	\
	download_lz4.rb				\
	download_mecab.rb			\
	download_message_pack.rb		\
	download_rapidjson.rb			\
	download.rb

dist-hook:
	GIT_DIR=$(srcdir)/mruby-source/.git git archive --format=tar HEAD | \
	  tar xf - -C $(distdir)/mruby-source
	cp -r mruby/mruby-include $(distdir)/
	$(MKDIR_P) $(distdir)/onigmo-source
	GIT_DIR=$(srcdir)/onigmo-source/.git git archive --format=tar HEAD | \
	  tar xf - -C $(distdir)/onigmo-source
	cd $(distdir)/onigmo-source && autoreconf --install --force
	cd $(distdir)/onigmo-source && sleep 1 && touch config.h.in
	$(MKDIR_P) $(distdir)/groonga-log-source
	GIT_DIR=$(srcdir)/groonga-log-source/.git git archive --format=tar HEAD | \
	  tar xf - -C $(distdir)/groonga-log-source
	cd $(distdir) && $(RUBY) download_rapidjson.rb
	cd $(distdir) && $(RUBY) download.rb blosc
	cd $(distdir) && $(RUBY) download.rb xsimd
