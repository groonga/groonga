#!/usr/bin/env ruby

require 'rubygems'
gem 'test-unit'
require 'test/unit'

exit Test::Unit::AutoRunner.run(true, File.dirname($0))
