#!/usr/bin/env ruby

require "rbconfig"
require "pathname"
require "fileutils"

base_dir_path       = Pathname(__FILE__).expand_path.dirname
source_top_dir_path = base_dir_path.parent.parent
build_top_dir_path  = Pathname($0).expand_path.dirname.parent.parent
build_base_dir_path = build_top_dir_path + "test/command_line"

Dir.chdir(build_top_dir_path.to_s) do
  system("make -j8 > /dev/null") or exit(false)
end

ENV["PATH"] = [
  (build_top_dir_path + "src").to_s,
  ENV["PATH"],
].join(File::PATH_SEPARATOR)
ENV["GRN_PLUGINS_DIR"]      = (build_top_dir_path + "plugins").to_s
ENV["GRN_RUBY_SCRIPTS_DIR"] = (build_top_dir_path + "lib/mrb/scripts").to_s

$VERBOSE = true

require_relative "helper"

ARGV.unshift("--max-diff-target-string-size=5000")

exit(Test::Unit::AutoRunner.run(true, (base_dir_path + "suite").to_s))
