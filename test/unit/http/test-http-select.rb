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

class HTTPSelectTest < Test::Unit::TestCase
  include GroongaHTTPTestUtils

  def setup
    setup_server
  end

  def teardown
    teardown_server
  end

  def test_select
    table_create("users",
                 :flags => Table::PAT_KEY,
                 :key_type => "ShortText")
    column_create("users", "real_name", Column::SCALAR, "ShortText")

    load("users", [{:_key => "ryoqun", :real_name => "Ryo Onodera"}])

    assert_select(["_id", "_key", "real_name"],
                  [[1, "ryoqun", "Ryo Onodera"]],
                  :table => "users",
                  :query => "real_name:\"Ryo Onodera\"")
  end

  def test_match_column
    populate_users

    assert_select(["_id", "_key", "real_name"],
                  [[2, "hayamiz", "Yuto Hayamizu"]],
                  :table => "users",
                  :match_column => "real_name",
                  :query => "Yuto Hayamizu")
  end

  def test_query
    populate_users

    assert_select(["_id", "_key", "real_name"],
                  [[2, "hayamiz", "Yuto Hayamizu"]],
                  :table => "users",
                  :query => "real_name:\"Yuto Hayamizu\"")
  end

  def test_filter
    populate_users

    assert_select(["_id", "_key", "real_name"],
                  [[2, "hayamiz", "Yuto Hayamizu"]],
                  :table => "users",
                  :filter => "real_name == \"Yuto Hayamizu\"")
  end

  def test_query_and_filter
    populate_users

    assert_select(["_id", "_key", "real_name"],
                  [[2, "hayamiz", "Yuto Hayamizu"]],
                  :table => "users",
                  :query => "real_name:\"Yuto Hayamizu\"",
                  :filter => "real_name == \"Yuto Hayamizu\"")
  end

  def test_no_hit
    populate_users

    assert_select(["_id", "_key", "real_name"],
                  [],
                  :table => "users",
                  :query => "real_name:\"No Name\"")
  end

  def test_scorer
    populate_users

    assert_select(["_id", "_key", "real_name"],
                  [[2, "hayamiz", "Real Name"],
                   [1, "ryoqun", "Real Name"]],
                  :table => "users",
                  :scorer => "real_name = \"Real Name\"")
  end

  def test_scorer_side_effect
    populate_users

    assert_select(["_id", "_key", "real_name"],
                  [[2, "hayamiz", "Real Name"]],
                  :table => "users",
                  :query => "real_name:\"Yuto Hayamizu\"",
                  :scorer => "real_name = \"Real Name\"")

    assert_select(["_id", "_key", "real_name"],
                  [],
                  :table => "users",
                  :query => "real_name:\"Yuto Hayamizu\"")
  end

  def test_output_columns
    populate_users

    assert_select(["real_name"],
                  [["Yuto Hayamizu"],
                   ["Ryo Onodera"]],
                  :table => "users",
                  :output_columns => "real_name")
  end

  def test_sortby
    create_bookmarks_table
    records = load_bookmarks((0...10).to_a.shuffle)

    assert_select(["_id", "_key"],
                  records.sort_by {|id, key| key},
                  :table => "bookmarks",
                  :sortby => "_key")
  end

  def test_sortby_reverse
    create_bookmarks_table
    records = load_bookmarks((0...10).to_a.shuffle)

    assert_select(["_id", "_key"],
                  records.sort_by {|id, key| key}.reverse,
                  :table => "bookmarks",
                  :sortby => "-_key")
  end

  def test_offset
    create_bookmarks_table
    records = load_bookmarks

    assert_select(["_id", "_key"],
                  records[3..-1],
                  {:table => "bookmarks", :offset => 3},
                  :n_hits => records.size)
  end

  def test_zero_offset
    create_bookmarks_table
    records = load_bookmarks

    assert_select(["_id", "_key"],
                  records,
                  {:table => "bookmarks", :offset => 0},
                  :n_hits => records.size)
  end

  def test_negative_offset
    create_bookmarks_table
    records = load_bookmarks

    assert_select(["_id", "_key"],
                  records[-3..-1],
                  {:table => "bookmarks", :offset => -3},
                  :n_hits => records.size)
  end

  def test_limit
    create_bookmarks_table
    records = load_bookmarks

    assert_select(["_id", "_key"],
                  records[0, 4],
                  {:table => "bookmarks", :limit => 4},
                  :n_hits => records.size)
  end

  def test_zero_limit
    create_bookmarks_table
    records = load_bookmarks

    assert_select(["_id", "_key"],
                  [],
                  {:table => "bookmarks", :limit => 0},
                  :n_hits => records.size)
  end

  def test_negative_limit
    create_bookmarks_table
    records = load_bookmarks

    assert_select(["_id", "_key"],
                  records,
                  {:table => "bookmarks", :limit => -1},
                  :n_hits => records.size)
  end

  def test_offset_and_limit
    create_bookmarks_table
    records = load_bookmarks

    assert_select(["_id", "_key"],
                  records[3, 4],
                  {:table => "bookmarks", :offset => 3, :limit => 4},
                  :n_hits => records.size)
  end

  def test_accessor
    create_users_table
    load_users
    create_comments_table
    load_comments

    assert_select(["_id", "text", "author"],
                  [[2, "Groonga rocks", "hayamiz"]],
                  :table => "comments",
                  :query => "author.real_name:\"Yuto Hayamizu\"")
  end

  def test_drilldown
    create_users_table
    load_users
    create_comments_table
    load_comments

    assert_select(["_id", "text", "author"],
                  [[1, "Ruby rocks", "ryoqun"],
                   [2, "Groonga rocks", "hayamiz"]],
                  {:table => "comments",
                   :drilldown => "author",
                   :drilldown_output_columns => "real_name",
                   :drilldown_limit => 10},
                  :expected_drilldown => [
                   [[2], ["real_name"], ["Ryo Onodera"], ["Yuto Hayamizu"]]])
  end

  def test_drilldown_sortby
    create_users_table
    load_many_users
    create_comments_table
    load_many_comments

    assert_drilldown({:drilldown_sortby => "_key"},
                     [["gunyara-kun"],
                      ["hayamiz"],
                      ["moritan"],
                      ["ryoqun"],
                      ["taporobo"]])
  end

  def test_drilldown_offset
    create_users_table
    load_many_users
    create_comments_table
    load_many_comments

    assert_drilldown({:drilldown_offset => 2},
                     [["gunyara-kun"],
                      ["moritan"],
                      ["ryoqun"]])
  end

  def test_drilldown_limit
    create_users_table
    load_many_users
    create_comments_table
    load_many_comments

    assert_drilldown({:drilldown_limit => 2},
                     [["taporobo"],
                      ["hayamiz"]])
  end

  def test_drilldown_offset_and_limit
    create_users_table
    load_many_users
    create_comments_table
    load_many_comments

    assert_drilldown({:drilldown_offset => 2,
                      :drilldown_limit => 1},
                     [["gunyara-kun"]])
  end

  private
  def assert_drilldown(options, values)
    assert_select(["_id", "text", "author"],
                  [[1, "ルビー最高！", "taporobo"],
                   [2, "グロンガ最高！", "hayamiz"],
                   [3, "Ruby/Groonga is useful.", "gunyara-kun"],
                   [4, "Ruby rocks!", "moritan"],
                   [5, "Groonga rocks!", "ryoqun"]],
                  {:table => "comments",
                   :drilldown => "author",
                   :drilldown_output_columns => "_key",
                   :drilldown_limit => 10}.merge(options),
                  :expected_drilldown => [
                   [[5],
                    ["_key"],
                    *values]])
  end
end
