# -*- coding: utf-8 -*-
#
# Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

require 'groonga-test-utils'

module GroongaLocalGQTPTestUtils
  include GroongaTestUtils

  def setup_local_database
    setup_database_path
    create_empty_database
  end

  def teardown_local_database
    teardown_database_path
  end

  private
  def create_empty_database
    run_groonga("-n", @database_path, "shutdown")
  end

  def dump
    output = run_groonga(@database_path, "dump")
    if $?.exitstatus != 255 # should groonga exit with 0, adhering the convention?
      flunk("groonga exited with unexpected exit status while dumping: " +
            " #{$?.exitstatus}")
    end
    output
  end

  def feed_commands(commands)
    output = ""
    IO.popen(construct_command_line(@database_path), "w+") do |pipe|
      pipe.write(commands)
      pipe.write("shutdown\n")
      output = pipe.read
    end
    unless $?.success?
      flunk("groonga exited with unexpected exit status while executing " +
            "commands: #{$?.exitstatus.inspect}:\n" +
            "commands:\n>>>\n#{commands}\n<<<\n" +
            "output:\n>>>\n#{output}\n<<<\n")
    end
    output.gsub(/^\[\[0,[\d\.e\-]+,[\d\.e\-]+\]/, "[[0,0.0,0.0]")
  end

  def assert_dump(expected, commands)
    feed_commands(commands)
    omit("dump test is now omitted.")
    assert_equal(expected, dump)
  end

  def assert_same_dump(commands)
    assert_dump(commands, commands)
  end

  def assert_commands(expected, commands)
    actual = feed_commands(commands)
    assert_equal(expected, actual)
  end
end
