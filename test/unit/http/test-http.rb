# -*- coding: utf-8 -*-
#
# Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>
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
  include GroongaTestUtils
  include GroongaHTTPTestUtils

  def setup
    setup_server
  end

  def teardown
    teardown_server
  end

  def test_root
    response = get("/")
    assert_equal("text/javascript", response.content_type)
    assert_equal("", response.body)
  end

  def test_status
    response = get(command_path(:status))
    assert_equal("text/javascript", response.content_type)
    assert_equal(["alloc_count", "starttime", "uptime"],
                 JSON.parse(response.body).keys.sort)
  end

  def test_table_list
    response = get(command_path(:table_list))
    assert_equal("text/javascript", response.content_type)
    assert_equal([["id", "name", "path", "flags", "domain"]],
                 JSON.parse(response.body).sort)

    response = get(command_path(:table_create,
                                :name => "users",
                                :flags => 1,
                                :key_type => "Int8",
                                :value_type => "Object",
                                :default_tokenizer => ""))
    assert_equal("true", response.body)

    response = get(command_path(:table_list))
    assert_equal("text/javascript", response.content_type)
    table_list = JSON.parse(response.body)
    header = table_list[0]
    body = table_list[1]
    assert_equal(2, table_list.length)
    assert_equal(["id", "name", "path", "flags", "domain"], header)
    table_name = body[1]
    assert_equal("users", table_name)
  end

  def test_column_list
    response = get(command_path(:table_create,
                                :name => "users",
                                :flags => 1,
                                :key_type => "Int8",
                                :value_type => "Object",
                                :default_tokenizer => ""))

    response = get(command_path(:column_list, :table => "users"))
    assert_equal("text/javascript", response.content_type)
    assert_equal([["id", "name", "path", "type", "flags", "domain"]],
                 JSON.parse(response.body).sort)

    response = get(command_path(:column_create,
                                :table => "users",
                                :name => "age",
                                :flags => 0,
                                :type => "Int8"))
    assert_equal("true", response.body)

    response = get(command_path(:column_list, :table => "users"))
    assert_equal("text/javascript", response.content_type)
    column_list = JSON.parse(response.body)
    assert_equal(2, column_list.length)
    header = column_list[0]
    body = column_list[1]
    assert_equal(["id", "name", "path", "type", "flags", "domain"], header)
    column_name = body[1]
    assert_equal("age", column_name)
  end

  def test_load
    response = get(command_path(:table_create,
                                :name => "users",
                                :flags => 1,
                                :key_type => "ShortText"))
    assert_equal("true", response.body)

    response = get(command_path(:column_create,
                                :table => "users",
                                :name => "real_name",
                                :flags => 0,
                                :type => "ShortText"))
    assert_equal("true", response.body)

    values = JSON.generate([{:_key => "ryoqun", :real_name => "Ryo Onodera"}])
    response = get(command_path(:load, :table => "users", :values => values))
    assert_equal("1", response.body)

    response = get(command_path(:select, :table => "users"))
    assert_equal("text/javascript", response.content_type)
    assert_equal([[0],
                 [[1],
                 ["_id","_key","real_name"],
                 [1,"ryoqun","Ryo Onodera"]
                 ]],
                 JSON.parse(response.body))
  end
end
