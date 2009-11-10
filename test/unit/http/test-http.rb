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
    assert_equal("text/javascript", response.content_type)
    assert_equal(["alloc_count", "starttime", "uptime"],
                 JSON.parse(response.body).keys.sort)
  end

  def test_load
    response = get(command_path(:table_create,
                                :name => "users",
                                :flags => Table::PAT_KEY,
                                :key_type => "ShortText"))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "text/javascript")

    response = get(command_path(:column_create,
                                :table => "users",
                                :name => "real_name",
                                :flags => Column::SCALAR,
                                :type => "ShortText"))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "text/javascript")

    values = JSON.generate([{:_key => "ryoqun", :real_name => "Ryo Onodera"}])
    response = get(command_path(:load, :table => "users", :values => values))
    assert_response([[Result::SUCCESS], 1], response,
                    :content_type => "text/javascript")

    response = get(command_path(:select, :table => "users"))
    assert_response([[Result::SUCCESS],
                     [[1],
                      ["_id", "_key", "real_name"],
                      [1, "ryoqun", "Ryo Onodera"]
                     ]],
                    response,
                    :content_type => "text/javascript")
  end

  def test_select
    response = get(command_path(:table_create,
                                :name => "users",
                                :flags => Table::PAT_KEY,
                                :key_type => "ShortText"))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "text/javascript")

    response = get(command_path(:column_create,
                                :table => "users",
                                :name => "real_name",
                                :flags => Column::SCALAR,
                                :type => "ShortText"))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "text/javascript")

    values = JSON.generate([{:_key => "ryoqun", :real_name => "Ryo Onodera"}])
    response = get(command_path(:load, :table => "users", :values => values))
    assert_response([[Result::SUCCESS], 1], response,
                    :content_type => "text/javascript")

    response = get(command_path(:select,
                                :table => "users",
                                :query => "real_name:\"Ryo Onodera\""))
    assert_response([[Result::SUCCESS],
                     [[1],
                      ["_id", "_key", "real_name"],
                      [1, "ryoqun", "Ryo Onodera"]
                     ]],
                    response,
                    :content_type => "text/javascript")
  end

  def test_select_match_column
    populate_users

    assert_select([[2, "hayamiz", "Yuto Hayamizu"]],
                  :table => "users",
                  :match_column => "real_name",
                  :query => "Yuto Hayamizu")
  end

  def test_select_query
    populate_users

    assert_select([[2, "hayamiz", "Yuto Hayamizu"]],
                  :table => "users",
                  :query => "real_name:\"Yuto Hayamizu\"")
  end

  def test_select_filter
    populate_users

    assert_select([[2, "hayamiz", "Yuto Hayamizu"]],
                  :table => "users",
                  :filter => "real_name == \"Yuto Hayamizu\"")
  end

  def test_select_scorer
    populate_users

    assert_select([[2, "hayamiz", "Real Name"],
                   [1, "ryoqun", "Real Name"]],
                  :table => "users",
                  :scorer => "real_name = \"Real Name\"")
  end

  def test_select_output_columns
    populate_users

    response = get(command_path(:select,
                                :table => "users",
                                :output_columns => "real_name"))
    assert_response([[Result::SUCCESS],
                     [[2],
                      ["real_name"],
                      ["Yuto Hayamizu"],
                      ["Ryo Onodera"]]],
                    response,
                    :content_type => "text/javascript")
  end

  def test_select_sortby
    create_bookmarks_table
    expected = load_shuffled_bookmarks

    response = get(command_path(:select,
                                :table => "bookmarks",
                                :sortby => "_key"))
    assert_response_range(expected, 0, 10, response)
  end

  def test_select_offset
    create_bookmarks_table
    expected = load_bookmarks

    response = get(command_path(:select,
                                :table => "bookmarks",
                                :offset => 3))
    assert_response_range(expected, 3, 10, response)
  end

  def test_select_limit
    create_bookmarks_table
    expected = load_bookmarks

    response = get(command_path(:select,
                                :table => "bookmarks",
                                :limit => 4))
    assert_response_range(expected, 0, 4, response)
  end

  def test_select_offset_and_limit
    create_bookmarks_table
    expected = load_bookmarks

    response = get(command_path(:select,
                                :table => "bookmarks",
                                :offset => 3,
                                :limit => 4))
    assert_response_range(expected, 3, 4, response)
  end

  private
  def assert_select(expected, parameters)
    response = get(command_path(:select, parameters))
    assert_response([[Result::SUCCESS],
                     [[expected.size],
                      ["_id", "_key", "real_name"],
                      *expected
                     ]],
                    response,
                    :content_type => "text/javascript")
  end

  def assert_response_range(expected, offset, limit, response)
    # expected[1]'s format:
    #   [<number of hits>, <header>, <record1>, <record2>...]
    expected[1] = expected[1][0, 2] + 
                  expected[1][offset + 2, limit]
    assert_response(expected, response, :content_type => "text/javascript")
  end
end
