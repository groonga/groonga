#!/usr/bin/env ruby

if ARGV.size != 5
  puts("Usage: #{$0} BUILD_COFNIG.RB MRUBY_SOURCE_DIR MRUBY_BUILD_DIR ONIGURUMA_INCLUDE_PATH TIMESTAMP_FILE")
  exit(false)
end

require "rbconfig"
require "fileutils"

build_config_rb = File.expand_path(ARGV.shift)
mruby_source_dir = ARGV.shift
mruby_build_dir = File.expand_path(ARGV.shift)
oniguruma_include_path = File.expand_path(ARGV.shift)
timestamp_file = File.expand_path(ARGV.shift)

FileUtils.rm_rf(mruby_build_dir)

unless system(RbConfig.ruby,
              "-S",
              "rake",
              "-f", "#{mruby_source_dir}/Rakefile",
              "--verbose",
              "MRUBY_CONFIG=#{build_config_rb}",
              "MRUBY_BUILD_DIR=#{mruby_build_dir}",
              "INSTALL_DIR=#{mruby_build_dir}/bin",
              "MRUBY_ONIGURUMA_INCLUDE_PATH=#{oniguruma_include_path}")
  exit(false)
end

FileUtils.touch(timestamp_file)

FileUtils.cp("#{mruby_build_dir}/host/LEGAL", "./")

FileUtils.cp("#{mruby_build_dir}/host/mrblib/mrblib.c", "./")

FileUtils.rm_rf("mruby-include")
include_dir = "#{mruby_build_dir}/host/include/"
# This directory doesn't exist on Windows because we disable presym on Windows.
# See also build_config.rb.
FileUtils.cp_r(include_dir, "mruby-include/") if File.exist?(include_dir)

File.open("mrbgems_init.c", "w") do |mrbgems_init|
  Dir.glob("#{mruby_build_dir}/host/mrbgems/**/gem_init.c") do |gem_init|
    mrbgems_init.puts(File.read(gem_init))
  end
end

mruby_dir_dir = "#{mruby_build_dir}/repos/host/mruby-dir"
FileUtils.mkdir_p("mruby-dir/")
FileUtils.cp_r("#{mruby_dir_dir}/src/", "mruby-dir/")

mruby_env_dir = "#{mruby_build_dir}/repos/host/mruby-env"
FileUtils.mkdir_p("mruby-env/")
FileUtils.cp_r("#{mruby_env_dir}/src/", "mruby-env/")

mruby_errno_dir = "#{mruby_build_dir}/repos/host/mruby-errno"
FileUtils.mkdir_p("mruby-errno/")
FileUtils.cp_r("#{mruby_errno_dir}/src/", "mruby-errno/")

mruby_file_stat_dir = "#{mruby_build_dir}/repos/host/mruby-file-stat"
FileUtils.mkdir_p("mruby-file-stat/")
FileUtils.cp_r("#{mruby_file_stat_dir}/src/", "mruby-file-stat/")

mruby_onig_regexp_dir = "#{mruby_build_dir}/repos/host/mruby-onig-regexp"
FileUtils.mkdir_p("mruby-onig-regexp/")
FileUtils.cp_r("#{mruby_onig_regexp_dir}/src/", "mruby-onig-regexp/")
