# -*- coding: utf-8 -*-
#
# Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>
# Copyright (C) 2009  Yuto HAYAMIZU <y.hayamizu@gmail.com>
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

class HTTPTest < Test::Unit::TestCase
  include GroongaHTTPTestUtils

  def setup
    setup_server
  end

  def teardown
    teardown_server
  end

  def test_status
    response = get(command_path(:status))
    assert_equal("application/json", response.content_type)
    assert_equal(["alloc_count", "starttime", "uptime"],
                 JSON.parse(response.body).keys.sort)
  end

  def test_load
    table_create("users",
                 :flags => Table::PAT_KEY,
                 :key_type => "ShortText")
    column_create("users", "real_name", Column::SCALAR, "ShortText")

    values = JSON.generate([{:_key => "ryoqun", :real_name => "Ryo Onodera"}])
    response = get(command_path(:load, :table => "users", :values => values))
    assert_response([[Result::SUCCESS], 1], response,
                    :content_type => "application/json")

    assert_select(["_id", "_key", "real_name"],
                  [[1, "ryoqun", "Ryo Onodera"]],
                  :table => "users")
  end

  def test_load_text_key_by_arrays
    create_users_table("ShortText")

    load("users", [[:_key], ["ryoqun"]])
    assert_select(["_id", "_key"],
                  [[1, "ryoqun"]],
                  :table => "users")
  end

  def test_load_text_key_by_objects
    create_users_table("ShortText")

    load("users", [{:_key => "ryoqun"}])
    assert_select(["_id", "_key"],
                  [[1, "ryoqun"]],
                  :table => "users")
  end

  def test_load_int_key_by_arrays
    create_users_table("Int32")

    load("users", [[:_key], [1000]])
    assert_select(["_id", "_key"],
                  [[1, 1000]],
                  :table => "users")
  end

  def test_load_int_key_by_objects
    create_users_table("Int32")

    load("users", [{:_key => 1000}])
    assert_select(["_id", "_key"],
                  [[1, 1000]],
                  :table => "users")
  end

  def test_load_int8_key
    create_users_table("Int8")
    assert_load_key
  end

  def test_load_int16_key
    create_users_table("Int16")
    assert_load_key
  end

  def test_load_int32_key
    create_users_table("Int32")
    assert_load_key
  end

  def test_load_int64_key
    create_users_table("Int64")
    assert_load_key
  end

  def test_load_int_value
    create_users_table("Int32", "Int32")

    load("users", [{:_key => 48, :_value => 45572}])
    assert_select(["_id", "_key", "_value"],
                  [[1, 48, 45572]],
                  :table => "users")
  end

  def test_quit
    response = get(command_path(:quit))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "application/json")

    assert_nothing_raised do
      get(command_path(:quit))
    end
  end

  def test_shutdown
    response = get(command_path(:shutdown))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "application/json")
    @groonga_pid = nil

    assert_raise(Errno::ECONNREFUSED) do
      get(command_path(:shutdown))
    end
  end

  private
  def create_users_table(key_type, value_type=nil)
    table_create("users",
                 :flags => Table::HASH_KEY,
                 :key_type => key_type,
                 :value_type => value_type)
  end

  def assert_load_key(user_id=48)
    load("users", [{:_key => user_id}])
    assert_select(["_id", "_key"],
                  [[1, user_id]],
                  :table => "users")
  end
end
