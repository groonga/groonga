# -*- coding: utf-8 -*-
#
# Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>
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

class HTTPLoadTest < Test::Unit::TestCase
  include GroongaHTTPTestUtils

  def setup
    setup_server
  end

  def teardown
    teardown_server
  end

  def test_columns
    create_users_table

    load("users",
         [["_key", "real_name"],
          ["ryoqun", "Ryo Onodera"],
          ["mori", "mori daijiro"]])

    assert_select(["_id", "_key", "real_name"],
                  [[1, "ryoqun", "Ryo Onodera"],
                   [2, "mori", "mori daijiro"]],
                  :table => "users")
  end

  def test_values
    create_users_table

    load("users",
         [{:_key => "ryoqun"},
          {:_key => "mori", :real_name => "mori daijiro"}])

    assert_select(["_id", "_key", "real_name"],
                  [[2, "mori", "mori daijiro"],
                   [1, "ryoqun", ""]],
                  :table => "users",
                  :sort_by => "_key")
  end

  def test_int8_key
    assert_key("Int8", 29)
  end

  def test_int16_key
    assert_key("Int16", 29)
  end

  def test_int32_key
    assert_key("Int32", 29)
  end

  def test_int64_key
    assert_key("Int64", 29)
  end

  def test_int_value
    table_create("int-hash",
                 :flags => Table::HASH_KEY,
                 :key_type => "Int32",
                 :value_type => "Int32")

    load("int-hash", [{:_key => 29, :_value => 10}])
    assert_select(["_id", "_key", "_value"],
                  [[1, 29, 10]],
                  :table => "int-hash")
  end

  private
  def create_users_table
    table_create("users",
                 :flags => Table::PAT_KEY,
                 :key_type => "ShortText")
    column_create("users", "real_name", Column::SCALAR, "ShortText")
  end

  def assert_key(key_type, key_value)
    table_name = "#{key_type}-hash"
    table_create(table_name,
                 :flags => Table::HASH_KEY,
                 :key_type => key_type)
    load(table_name, [{:_key => key_value}])
    assert_select(["_id", "_key"],
                  [[1, key_value]],
                  :table => table_name)
  end
end
