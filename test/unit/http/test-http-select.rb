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
    create_user_id_table
    records = load_user_ids((0...10).to_a.shuffle)

    assert_select(["_id", "_key"],
                  records.sort_by {|id, key| key},
                  :table => "user_id",
                  :sortby => "_key")
  end

  def test_sortby_reverse
    create_user_id_table
    records = load_user_ids((0...10).to_a.shuffle)

    assert_select(["_id", "_key"],
                  records.sort_by {|id, key| key}.reverse,
                  :table => "user_id",
                  :sortby => "-_key")
  end

  def test_sortby_with_multiple_column
    create_calendar_table
    records = load_schedules

    assert_select(["_id","month","day"],
                  records.sort_by {|id, month, day| [month, day]},
                  :table => "calendar",
                  :limit => -1,
                  :sortby => "month day")
  end

  def test_offset
    create_user_id_table
    records = load_user_ids

    assert_select(["_id", "_key"],
                  records[3..-1],
                  {:table => "user_id", :offset => 3},
                  :n_hits => records.size)
  end

  def test_zero_offset
    create_user_id_table
    records = load_user_ids

    assert_select(["_id", "_key"],
                  records,
                  {:table => "user_id", :offset => 0},
                  :n_hits => records.size)
  end

  def test_negative_offset
    create_user_id_table
    records = load_user_ids

    assert_select(["_id", "_key"],
                  records[-3..-1],
                  {:table => "user_id", :offset => -3},
                  :n_hits => records.size)
  end

  def test_offset_one_larger_than_hits
    create_user_id_table
    records = load_user_ids

    response = get(command_path(:select, :table => "user_id", :offset => 11))
    assert_response([[Result::INVALID_ARGUMENT,
                      "too large offset"]],
                    response,
                    :content_type => "application/json")
  end

  def test_negative_offset_one_larger_than_hits
    create_user_id_table
    records = load_user_ids

    response = get(command_path(:select, :table => "user_id", :offset => -11))
    assert_response([[Result::INVALID_ARGUMENT,
                      "too small negative offset"]],
                    response,
                    :content_type => "application/json")
  end

  def test_offset_equal_to_hits
    create_user_id_table
    records = load_user_ids

    assert_select(["_id", "_key"],
                  [],
                  {:table => "user_id", :offset => 10},
                  :n_hits => records.size)
  end

  def test_negative_offset_equal_to_hits
    create_user_id_table
    records = load_user_ids

    assert_select(["_id", "_key"],
                  records,
                  {:table => "user_id", :offset => -10},
                  :n_hits => records.size)
  end

  def test_limit
    create_user_id_table
    records = load_user_ids

    assert_select(["_id", "_key"],
                  records[0, 4],
                  {:table => "user_id", :limit => 4},
                  :n_hits => records.size)
  end

  def test_zero_limit
    create_user_id_table
    records = load_user_ids

    assert_select(["_id", "_key"],
                  [],
                  {:table => "user_id", :limit => 0},
                  :n_hits => records.size)
  end

  def test_negative_limit
    create_user_id_table
    records = load_user_ids

    assert_select(["_id", "_key"],
                  records,
                  {:table => "user_id", :limit => -1},
                  :n_hits => records.size)
  end

  def test_offset_and_limit
    create_user_id_table
    records = load_user_ids

    assert_select(["_id", "_key"],
                  records[3, 4],
                  {:table => "user_id", :offset => 3, :limit => 4},
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

  def test_multiple_drilldown
    create_users_table
    load_users
    create_comments_table
    load_comments

    assert_select(["_id", "text", "author"],
                  [[1, "Ruby rocks", "ryoqun"],
                   [2, "Groonga rocks", "hayamiz"]],
                  {:table => "comments",
                   :drilldown => "text author",
                   :drilldown_output_columns => "_key",
                   :drilldown_limit => 10},
                  :expected_drilldown => [
                   [[2], ["_key"], ["Ruby rocks"], ["Groonga rocks"]],
                   [[2], ["_key"], ["ryoqun"], ["hayamiz"]]])
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

  def test_zero_drilldown_offset
    create_users_table
    load_many_users
    create_comments_table
    load_many_comments

    assert_drilldown({:drilldown_offset => 0},
                     [["taporobo"],
                      ["hayamiz"],
                      ["gunyara-kun"],
                      ["moritan"],
                      ["ryoqun"]])
  end

  def test_negative_drilldown_offset
    create_users_table
    load_many_users
    create_comments_table
    load_many_comments

    assert_drilldown({:drilldown_offset => -2},
                     [["moritan"],
                      ["ryoqun"]])
  end

  def test_drilldown_offset_one_larger_than_hits
    create_users_table
    load_many_users
    create_comments_table
    load_many_comments

    response = get(command_path(:select,
                                :table => "comments",
                                :drilldown => "author",
                                :drilldown_output_columns => "_key",
                                :drilldown_limit => 10,
                                :drilldown_offset => 6))
    assert_response([[Result::INVALID_ARGUMENT,
                      "too large drilldown_offset"]],
                    response,
                    :content_type => "application/json")
  end

  def test_negative_drilldown_offset_one_larger_than_hits
    create_users_table
    load_many_users
    create_comments_table
    load_many_comments

    response = get(command_path(:select,
                                :table => "comments",
                                :drilldown => "author",
                                :drilldown_output_columns => "_key",
                                :drilldown_limit => 10,
                                :drilldown_offset => -6))
    assert_response([[Result::INVALID_ARGUMENT,
                      "too small negative drilldown_offset"]],
                    response,
                    :content_type => "application/json")
  end

  def test_drilldown_offset_equal_to_hits
    create_users_table
    load_many_users
    create_comments_table
    load_many_comments

    assert_drilldown({:drilldown_offset => 5}, [])
  end

  def test_negative_drilldwon_offset_equal_to_hits
    create_users_table
    load_many_users
    create_comments_table
    load_many_comments

    assert_drilldown({:drilldown_offset => -5},
                     [["taporobo"],
                      ["hayamiz"],
                      ["gunyara-kun"],
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

  def test_zero_drilldown_limit
    create_users_table
    load_many_users
    create_comments_table
    load_many_comments

    assert_drilldown({:drilldown_limit => 0}, [])
  end

  def test_negative_drilldown_limit
    create_users_table
    load_many_users
    create_comments_table
    load_many_comments

    assert_drilldown({:drilldown_limit => -3},
                     [["taporobo"],
                      ["hayamiz"],
                      ["gunyara-kun"],
                      ["moritan"],
                      ["ryoqun"]])

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
                  [[1, "Ruby最高！", "taporobo"],
                   [2, "Groonga最高！", "hayamiz"],
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
