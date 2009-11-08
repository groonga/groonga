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
  TABLE_HASH_KEY = 0x0
  TABLE_PAT_KEY  = 0x1
  TABLE_NO_KEY = 0x3
  TABLE_VIEW = 0x04
  KEY_WITH_SIS = 0x40
  KEY_NORMALIZE = 0x80

  COLUMN_SCALAR = 0x0
  COLUMN_VECTOR = 0x1
  COLUMN_INDEX = 0x2
  WITH_SECTION = 0x80
  WITH_WEIGHT = 0x100
  WITH_POSITION = 0x200

  def setup
    setup_server
  end

  def teardown
    teardown_server
  end

  def test_static_html
    response = get("/index.html")
    assert_equal("text/html", response.content_type)
    assert_equal(File.read(File.join(@resource_dir, "index.html")),
                 response.body)
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
                 JSON.parse(response.body))

    response = get(command_path(:table_create,
                                :name => "users",
                                :flags => TABLE_PAT_KEY,
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
                                :flags => TABLE_PAT_KEY,
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
                                :flags => COLUMN_SCALAR,
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
                                :flags => TABLE_PAT_KEY,
                                :key_type => "ShortText"))
    assert_equal("true", response.body)

    response = get(command_path(:column_create,
                                :table => "users",
                                :name => "real_name",
                                :flags => COLUMN_SCALAR,
                                :type => "ShortText"))
    assert_equal("true", response.body)

    values = JSON.generate([{:_key => "ryoqun", :real_name => "Ryo Onodera"}])
    response = get(command_path(:load, :table => "users", :values => values))
    assert_equal("1", response.body)

    response = get(command_path(:select, :table => "users"))
    assert_equal("text/javascript", response.content_type)
    assert_equal([[0],
                  [[1],
                   ["_id", "_key", "real_name"],
                   [1, "ryoqun", "Ryo Onodera"]
                  ]],
                 JSON.parse(response.body))
  end

  def test_select
    response = get(command_path(:table_create,
                                :name => "users",
                                :flags => TABLE_PAT_KEY,
                                :key_type => "ShortText"))
    assert_equal("true", response.body)

    response = get(command_path(:column_create,
                                :table => "users",
                                :name => "real_name",
                                :flags => COLUMN_SCALAR,
                                :type => "ShortText"))
    assert_equal("true", response.body)

    values = JSON.generate([{:_key => "ryoqun", :real_name => "Ryo Onodera"}])
    response = get(command_path(:load, :table => "users", :values => values))
    assert_equal("1", response.body)

    response = get(command_path(:select,
                                :table => "users",
                                :query => "real_name:\"Ryo Onodera\""))
    assert_equal([[0],
                  [[1],
                   ["_id", "_key", "real_name"],
                   [1, "ryoqun", "Ryo Onodera"]
                  ]],
                 JSON.parse(response.body))
  end

  def test_select_match_column
    create_users_table

    response = get(command_path(:select,
                                :table => "users",
                                :match_column => "real_name",
                                :query => "Yuto Hayamizu"))
    assert_equal("text/javascript", response.content_type)
    assert_select(response)
  end

  def test_select_query
    create_users_table

    response = get(command_path(:select,
                                :table => "users",
                                :query => "real_name:\"Yuto Hayamizu\""))
    assert_equal("text/javascript", response.content_type)
    assert_select(response)
  end

  def test_select_filter
    create_users_table

    response = get(command_path(:select,
                                :table => "users",
                                :filter => "real_name == \"Yuto Hayamizu\""))
    assert_equal("text/javascript", response.content_type)
    assert_select(response)
  end

  private
  def create_users_table
    response = get(command_path(:table_create,
                                :name => "users",
                                :flags => TABLE_PAT_KEY,
                                :key_type => "ShortText"))
    assert_equal("true", response.body)

    response = get(command_path(:column_create,
                                :table => "users",
                                :name => "real_name",
                                :flags => COLUMN_SCALAR,
                                :type => "ShortText"))
    assert_equal("true", response.body)

    response = get(command_path(:table_create,
                                :name => "terms",
                                :flags => TABLE_PAT_KEY,
                                :key_type => "ShortText",
                                :default_tokenizer => "TokenBigram"))
    assert_equal("true", response.body)

    response = get(command_path(:column_create,
                                :table => "terms",
                                :name => "users_real_name",
                                :flags => COLUMN_INDEX,
                                :type => "users",
                                :source => "real_name"))
    assert_equal("true", response.body)

    values = JSON.generate([{:_key => "ryoqun", :real_name => "Ryo Onodera"}])
    response = get(command_path(:load, :table => "users", :values => values))
    assert_equal("1", response.body)

    values = JSON.generate([{:_key => "hayamiz", :real_name => "Yuto Hayamizu"}])
    response = get(command_path(:load, :table => "users", :values => values))
    assert_equal("1", response.body)
  end

  def assert_select(response)
    assert_equal([[0],
                  [[1],
                   ["_id", "_key", "real_name"],
                   [2, "hayamiz", "Yuto Hayamizu"]
                  ]],
                 JSON.parse(response.body))
  end
end
