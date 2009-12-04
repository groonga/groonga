# -*- coding: utf-8 -*-
#
# Copyright (C) 2009  Ryo Onodera <onodera@clear-code.com>
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

class DumpTest < Test::Unit::TestCase
  include GroongaTestUtils

  def setup
    @tmp_dir = File.join(File.dirname(__FILE__), "tmp")
    FileUtils.rm_rf(@tmp_dir)
    FileUtils.mkdir_p(@tmp_dir)
    @database_path = File.join(@tmp_dir, "database")
  end

  def teardown
    FileUtils.rm_rf(@tmp_dir)
  end

  def test_hash_table_create
    assert_dump("table_create Blog 0 ShortText\n")
  end

  def test_patricia_table_create
    assert_dump("table_create Blog 1 ShortText\n")
  end

  def test_no_key_table_create
    assert_dump("table_create Blog 3\n")
  end

  def test_view_table_create
    assert_dump("table_create Blog 4\n")
  end

  def test_table_create_key_normalize
    assert_dump("table_create Blog 128 ShortText\n")
  end

  def test_table_create_with_value_type
    assert_dump("table_create Blog 128 ShortText Int32\n")
  end

  def test_table_create_escaped_string
    assert_dump("table_create \"Blog\\\"\" 0 ShortText\n")
  end

  def test_multiple_table_create
    assert_dump("table_create users 0 ShortText\n" +
                "table_create admin_users 0 users\n")
  end

  def test_order_of_table_create
    assert_dump(('a'..'z').to_a.shuffle.collect do |letter|
                  "table_create #{letter} 0 ShortText\n"
                end.join)
  end

  private
  def dump
    run_groonga(@database_path, "dump")
  end

  def feed_commands(commands)
    IO.popen(construct_command_line("-n", @database_path), "w+") do |pipe|
      pipe.write(commands)
      pipe.write("shutdown\n")
      pipe.read
    end
  end

  def assert_dump(expected)
    feed_commands(expected)
    assert_equal(expected, dump)
  end
end
