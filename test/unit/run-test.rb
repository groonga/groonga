#!/usr/bin/env ruby

$KCODE = 'utf-8' unless defined?(Encoding)

require 'rbconfig'
require 'fileutils'

build_dir = File.expand_path(ENV["BUILD_DIR"] || File.dirname(__FILE__))
base_dir = File.expand_path(ENV["BASE_DIR"] || File.dirname(__FILE__))

test_lib_dir = File.expand_path(File.join(build_dir, "..", "lib"))
FileUtils.mkdir_p(test_lib_dir)

test_unit_dir = File.join(test_lib_dir, "test-unit-2.1.1")
unless File.exist?(test_unit_dir)
  require "open-uri"
  tgz_uri = "http://rubyforge.org/frs/download.php/71835/test-unit-2.1.1.tgz"
  tgz = File.join(build_dir, File.basename(tgz_uri))
  File.open(tgz, "wb") do |output|
    output.print(open(tgz_uri).read)
  end
  system("tar", "xfz", tgz, "-C", test_lib_dir)
end
$LOAD_PATH.unshift(File.join(test_unit_dir, "lib"))

test_unit_notify_dir = File.join(test_lib_dir, "test-unit-notify-0.0.1")
unless File.exist?(test_unit_notify_dir)
  require "open-uri"
  tgz_uri = "http://rubyforge.org/frs/download.php/71705/test-unit-notify-0.0.1.tgz"
  tgz = File.join(build_dir, File.basename(tgz_uri))
  File.open(tgz, "wb") do |output|
    output.print(open(tgz_uri).read)
  end
  system("tar", "xfz", tgz, "-C", test_lib_dir)
end
$LOAD_PATH.unshift(File.join(test_unit_notify_dir, "lib"))

require 'test/unit'
require 'test/unit/version'
require 'test/unit/notify'

json_dir = File.join(test_lib_dir, "json-1.1.9")
unless File.exist?(json_dir)
  require "open-uri"
  require "fileutils"
  tgz_uri = "http://rubyforge.org/frs/download.php/62984/json-1.1.9.tgz"
  tgz = File.join(build_dir, File.basename(tgz_uri))
  File.open(tgz, "wb") do |output|
    output.print(open(tgz_uri).read)
  end
  system("tar", "xfz", tgz, "-C", test_lib_dir)
  ext_parser_dir = File.join(json_dir, "ext", "json", "ext", "parser")
  Dir.chdir(ext_parser_dir) do
    ruby = File.join(RbConfig::CONFIG["bindir"],
                     RbConfig::CONFIG["ruby_install_name"])
    system(ruby, "extconf.rb")
    system("make")
  end
  FileUtils.mv(File.join(ext_parser_dir, "parser.so"),
               File.join(json_dir, "lib", "json", "ext"))
end
$LOAD_PATH.unshift(File.join(json_dir, "lib"))


$LOAD_PATH.unshift(File.join(base_dir, "lib", "ruby"))

require 'groonga-test-utils'
require 'groonga-http-test-utils'
require 'groonga-local-gqtp-test-utils'

ARGV.unshift("--exclude", "run-test.rb")
ARGV.unshift("--notify")
exit Test::Unit::AutoRunner.run(true, File.dirname($0))
