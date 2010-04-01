#!/usr/bin/env ruby

$KCODE = 'utf-8'

require 'fileutils'

base_dir = File.expand_path(File.dirname(__FILE__))

test_lib_dir = File.expand_path(File.join(base_dir, "..", "lib"))
FileUtils.mkdir_p(test_lib_dir)

test_unit_dir = File.join(test_lib_dir, "test-unit-2.0.7")
unless File.exist?(test_unit_dir)
  require "open-uri"
  tgz_uri = "http://rubyforge.org/frs/download.php/69597/test-unit-2.0.7.tgz"
  tgz = File.join(base_dir, File.basename(tgz_uri))
  File.open(tgz, "wb") do |output|
    output.print(open(tgz_uri).read)
  end
  system("tar", "xfz", tgz, "-C", test_lib_dir)
end

$LOAD_PATH.unshift(File.join(test_unit_dir, "lib"))

require 'test/unit'

json_dir = File.join(test_lib_dir, "json-1.1.9")
unless File.exist?(json_dir)
  require "open-uri"
  require "fileutils"
  tgz_uri = "http://rubyforge.org/frs/download.php/62984/json-1.1.9.tgz"
  tgz = File.join(base_dir, File.basename(tgz_uri))
  File.open(tgz, "wb") do |output|
    output.print(open(tgz_uri).read)
  end
  system("tar", "xfz", tgz, "-C", test_lib_dir)
  ext_parser_dir = File.join(json_dir, "ext", "json", "ext", "parser")
  Dir.chdir(ext_parser_dir) do
    system("ruby", "extconf.rb")
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

exit Test::Unit::AutoRunner.run(true, File.dirname($0))
