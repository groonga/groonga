#!/usr/bin/env ruby

require "rbconfig"
require "pathname"
require "fileutils"

source_base_dir_path = Pathname(__dir__).expand_path
source_top_dir_path = source_base_dir_path.parent.parent
if ENV["BUILD_DIR"]
  build_base_dir_path = Pathname(ENV["BUILD_DIR"]).expand_path
else
  build_base_dir_path  = Pathname($0).expand_path.dirname
end
build_top_dir_path = build_base_dir_path.parent.parent

ENV["BASE_DIR"] ||= build_base_dir_path.to_s

groonga_install_prefix = nil
if (ARGV[0] || "").start_with?("--groonga-install-prefix=")
  groonga_install_prefix = ARGV.shift.gsub(/\A--groonga-install-prefix=/, "")
end

if groonga_install_prefix
  unescaped_prefix = groonga_install_prefix.gsub(/\\u([\da-fA-F]{4})/) { [$1].pack('H*').unpack('n*').pack("U*")}
  ENV["PATH"] = [
    [groonga_install_prefix, "bin"].join(File::ALT_SEPARATOR || File::SEPARATOR),
    [unescaped_prefix, "bin"].join(File::ALT_SEPARATOR || File::SEPARATOR),
    ENV["PATH"],
  ].join(File::PATH_SEPARATOR)
else
  Dir.chdir(build_top_dir_path.to_s) do
    if File.exist?("Makefile")
      system("make -j8 > /dev/null") or exit(false)
    end
  end

  ENV["PATH"] = [
    (build_top_dir_path + "src").to_s,
    ENV["PATH"],
  ].join(File::PATH_SEPARATOR)
  if build_top_dir_path != source_top_dir_path
    Dir.glob(source_top_dir_path + "plugins/**/*.rb") do |source_rb|
      relative_path = Pathname(source_rb).relative_path_from(source_top_dir_path)
      build_rb = build_top_dir_path + relative_path
      FileUtils.mkdir_p(build_rb.dirname)
      FileUtils.cp(source_rb, build_rb)
    end
  end
  ENV["GRN_PLUGINS_DIR"]      = (build_top_dir_path + "plugins").to_s
  ENV["GRN_RUBY_SCRIPTS_DIR"] = (source_top_dir_path + "lib/mrb/scripts").to_s
  ENV["GRN_RUBY_LOAD_PATH"] = [
    (source_top_dir_path + "vendor/groonga-log-source/lib").to_s,
    ENV["GRN_RUBY_LOAD_PATH"],
  ].compact.join(File::PATH_SEPARATOR)
end

$VERBOSE = true

require_relative "helper"

ARGV.unshift("--max-diff-target-string-size=5000")

exit(Test::Unit::AutoRunner.run(true, (source_base_dir_path + "suite").to_s))
