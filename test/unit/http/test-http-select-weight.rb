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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

class HTTPSelectWeightTest < Test::Unit::TestCase
  include GroongaHTTPTestUtils

  def setup
    setup_server
  end

  def teardown
    teardown_server
  end

  def test_weight
    populate_users

    assert_select([["_key", "ShortText"],
                   ["_score", "Int32"]],
                  [["hayamiz", 10]],
                  :table => "users",
                  :output_columns => "_key _score",
                  :match_columns => "real_name * 10",
                  :query => "Yuto")
  end

  def test_weight_minus
    omit("Is this test right?")
    populate_users

    assert_select([["_key", "ShortText"],
                   ["_score", "Int32"]],
                  [["hayamiz", -10]],
                  :table => "users",
                  :output_columns => "_key _score",
                  :match_columns => "real_name * -10",
                  :query => "Yuto")
  end

  def test_multi_match_columns
    create_users_table
    load_many_users

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"],
                   ["_score", "Int32"]],
                  [[2, "taporobo", 15],
                   [1, "moritan", 11]],
                  :table => "users",
                  :match_columns => "real_name * 1 || description * 5",
                  :sortby => '-_score',
                  :output_columns => "_id,_key,_score",
                  :query => "モリ")

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"],
                   ["_score", "Int32"]],
                  [[1, "moritan", 7],
                   [2, "taporobo", 3]],
                  :table => "users",
                  :match_columns => "real_name * 5 || description * 1",
                  :sortby => '-_score',
                  :output_columns => "_id,_key,_score",
                  :query => "モリ")
  end

  def test_multi_match_columns_without_index_partial
    omit('not handled properly')

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"],
                   ["real_name", "ShortText"]],
                  [[2, "hayamiz", "Yuto Hayamizu"]],
                  :table => "users",
                  :match_columns => "_key || real_name",
                  :output_columns => "_id,_key,real_name",
                  :query => "Yuto")
  end

  def test_multi_columns_index_multi_columns_search
    create_users_table
    load_many_users

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"]],
                  [[1, "moritan"],
                   [2, "taporobo"]],
                  :table => "users",
                  :match_columns => "prefecture || city",
                  :sortby => '-_score',
                  :output_columns => "_id,_key",
                  :query => "モリ")
  end

  def test_multi_columns_index_single_column_search
    create_users_table
    load_many_users

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"]],
                  [[4, "hayamiz"]],
                  :table => "users",
                  :match_columns => "prefecture",
                  :sortby => '_id',
                  :output_columns => "_id,_key",
                  :query => "富山県")

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"]],
                  [[2, "taporobo"]],
                  :table => "users",
                  :match_columns => "city",
                  :sortby => '_id',
                  :output_columns => "_id,_key",
                  :query => "タポロボ市")
  end
end
