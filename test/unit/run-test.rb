#!/usr/bin/env ruby

require 'rubygems'
gem 'test-unit'
require 'test/unit'
require 'test/unit/version'

base_dir = File.expand_path(File.dirname(__FILE__))
$LOAD_PATH.unshift(File.join(base_dir, "lib", "ruby"))

require 'groonga-test-utils'

if Test::Unit::VERSION <= "2.0.5"
  Dir.glob(File.join(File.dirname(__FILE__), "**", "test-*.rb")) do |file|
    require file.sub(/\.rb$/, '')
  end
  exit Test::Unit::AutoRunner.run
else
  exit Test::Unit::AutoRunner.run(true, File.dirname($0))
end
