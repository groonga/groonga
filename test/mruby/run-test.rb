#!/usr/bin/env ruby

require "rbconfig"
require "pathname"
require "fileutils"

base_dir_path       = Pathname(__FILE__).expand_path.dirname
source_top_dir_path = base_dir_path.parent.parent
build_top_dir_path  = Pathname($0).expand_path.dirname.parent.parent
build_base_dir_path = build_top_dir_path + "test/mruby"

Dir.chdir(build_top_dir_path.to_s) do
  system("make -j8 > /dev/null") or exit(false)
end

rroonga_built_revision_path = build_base_dir_path + "rroonga.built"
rroonga_built_revision = nil
if rroonga_built_revision_path.exist?
  rroonga_built_revision = rroonga_built_revision_path.read
end

rroonga_dir_path = build_base_dir_path + "rroonga"
rroonga_revision = Dir.chdir(rroonga_dir_path) do
  `git describe`
end
if rroonga_revision != rroonga_built_revision
  groonga_pc_path = build_base_dir_path + "groonga.pc"
  groonga_pc_path.open("w") do |groonga_pc|
    content = (build_top_dir_path + "groonga.pc").read
    content = content.gsub(/^Libs: .*$/) do
      lib_dir = "#{build_top_dir_path}/lib/.libs"
      "Libs: -L#{lib_dir} -Wl,-rpath,#{lib_dir} -lgroonga"
    end
    content = content.gsub(/^Cflags: .*$/) do
      "Cflags: -I#{build_top_dir_path}/include"
    end
    groonga_pc.puts(content)
  end
  pkg_config_path = (ENV["PKG_CONFIG_PATH"] || "").split(File::PATH_SEPARATOR)
  pkg_config_path.unshift(groonga_pc_path.dirname.to_s)
  ENV["PKG_CONFIG_PATH"] = pkg_config_path.join(File::PATH_SEPARATOR)
  Dir.chdir(rroonga_dir_path.to_s) do
    system("make", "clean") if File.exist?("Makefile")
    system(RbConfig.ruby,
           "extconf.rb",
           "--enable-debug-build")
    system("make", "-j")
  end
  rroonga_built_revision_path.open("w") do |file|
    file.print(rroonga_revision)
  end
end

ENV["GRN_PLUGINS_DIR"] = (build_top_dir_path + "plugins").to_s
ENV["GRN_RUBY_SCRIPTS_DIR"] = (build_top_dir_path + "lib/mrb/scripts").to_s
ENV["GRN_ORDER_BY_ESTIMATED_SIZE_ENABLE"] = "yes"

$LOAD_PATH.unshift((rroonga_dir_path + "ext" + "groonga").to_s)
$LOAD_PATH.unshift((rroonga_dir_path + "lib").to_s)

require "groonga"

require_relative "helper"

exit(Test::Unit::AutoRunner.run(true, (base_dir_path + "suite").to_s))
