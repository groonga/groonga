# -*- coding: utf-8 -*-
#
# Copyright (C) 2009-2010  Kouhei Sutou <kou@clear-code.com>
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

module HTTPSelectBasicTests
  include GroongaHTTPTestUtils

  def setup
    setup_server
  end

  def teardown
    teardown_server
  end

  def test_match_columns
    populate_users

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"],
                   ["real_name", "ShortText"]],
                  [[2, "hayamiz", "Yuto Hayamizu"]],
                  {:table => "users",
                   :match_columns => "real_name",
                   :output_columns => "_id,_key,real_name",
                   :query => "Yuto Hayamizu",
                   :output_type => output_type},
                  :content_type => content_type)
  end

  def test_query
    populate_users

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"],
                   ["real_name", "ShortText"]],
                  [[2, "hayamiz", "Yuto Hayamizu"]],
                  {:table => "users",
                   :output_columns => "_id,_key,real_name",
                   :query => "real_name:\"Yuto Hayamizu\"",
                   :output_type => output_type},
                  :content_type => content_type)
  end

  def test_filter
    populate_users

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"],
                   ["real_name", "ShortText"]],
                  [[2, "hayamiz", "Yuto Hayamizu"]],
                  {:table => "users",
                   :output_columns => "_id,_key,real_name",
                   :filter => "real_name == \"Yuto Hayamizu\"",
                   :output_type => output_type},
                  :content_type => content_type)
  end

  def test_query_and_filter
    populate_users

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"],
                   ["real_name", "ShortText"]],
                  [[2, "hayamiz", "Yuto Hayamizu"]],
                  {:table => "users",
                   :output_columns => "_id,_key,real_name",
                   :query => "real_name:\"Yuto Hayamizu\"",
                   :filter => "real_name == \"Yuto Hayamizu\"",
                   :output_type => output_type},
                  :content_type => content_type)
  end

  def test_no_hit
    populate_users

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"],
                   ["real_name", "ShortText"]],
                  [],
                  {:table => "users",
                   :output_columns => "_id,_key,real_name",
                   :query => "real_name:\"No Name\"",
                   :output_type => output_type},
                  :content_type => content_type)
  end

  def test_scorer
    populate_users

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"],
                   ["real_name", "ShortText"]],
                  [[2, "hayamiz", "Real Name"],
                   [1, "ryoqun", "Real Name"]],
                  {:table => "users",
                   :output_columns => "_id,_key,real_name",
                   :scorer => "real_name = \"Real Name\"",
                   :output_type => output_type},
                  :content_type => content_type)
  end

  def test_scorer_side_effect
    populate_users

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"],
                   ["real_name", "ShortText"]],
                  [[2, "hayamiz", "Real Name"]],
                  {:table => "users",
                   :output_columns => "_id,_key,real_name",
                   :query => "real_name:\"Yuto Hayamizu\"",
                   :scorer => "real_name = \"Real Name\"",
                   :output_type => output_type},
                  :content_type => content_type)

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"],
                   ["real_name", "ShortText"]],
                  [],
                  {:table => "users",
                   :output_columns => "_id,_key,real_name",
                   :query => "real_name:\"Yuto Hayamizu\"",
                   :output_type => output_type},
                  :content_type => content_type)
  end

  def test_output_columns
    populate_users

    assert_select([["real_name", "ShortText"]],
                  [["Yuto Hayamizu"],
                   ["Ryo Onodera"]],
                  {:table => "users",
                   :output_columns => "real_name",
                   :output_type => output_type},
                  :content_type => content_type)
  end

  def test_output_columns_wild_card
    populate_users

    assert_select([["_key", "ShortText"],
                   ["city", "ShortText"],
                   ["description", "ShortText"],
                   ["hp", "Int32"],
                   ["prefecture", "ShortText"],
                   ["real_name", "ShortText"]],
                  [["hayamiz", "富山市", "λかわいいよλ", 200, "富山県", "Yuto Hayamizu"],
                   ["ryoqun", "不明", "ryoくんです。", 200, "不明", "Ryo Onodera"]],
                   {:table => "users",
                    :output_columns => "_key *",
                    :output_type => output_type},
                   :content_type => content_type)
  end

  def test_output_columns_nonexistent
    populate_users

    assert_select([["real_name", "ShortText"]],
                  [["Yuto Hayamizu"],
                   ["Ryo Onodera"]],
                  {:table => "users",
                   :output_columns => "nonexistent real_name",
                   :output_type => output_type},
                  :content_type => content_type)
  end

  def test_sortby
    create_user_id_table
    records = register_users((0...10).to_a.shuffle)

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  records.sort_by {|id, key| key},
                  {:table => "user_id",
                   :sortby => "_key",
                   :output_type => output_type},
                  :content_type => content_type)
  end

  def test_sortby_reverse
    create_user_id_table
    records = register_users((0...10).to_a.shuffle)

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  records.sort_by {|id, key| key}.reverse,
                  {:table => "user_id",
                   :sortby => "-_key",
                   :output_type => output_type},
                  :content_type => content_type)
  end

  def test_sortby_with_multiple_column
    create_calendar_table
    records = load_schedules

    assert_select([["_id", "UInt32"],
                   ["day", "Int32"],
                   ["month", "Int32"]],
                  records.sort_by {|id, day, month| [month, day]},
                  {:table => "calendar",
                   :limit => -1,
                   :sortby => "month day",
                   :output_type => output_type},
                  :content_type => content_type)
  end

  def test_sortby_offset
    create_user_id_table
    records = register_users((0...10).to_a.shuffle)

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  records.sort_by {|id, key| key}[3..-1],
                  {:table => "user_id",
                   :sortby => "_key",
                   :offset => 3,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_sortby_zero_offset
    create_user_id_table
    records = register_users((0...10).to_a.shuffle)

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  records.sort_by {|id, key| key},
                  {:table => "user_id",
                   :sortby => "_key",
                   :offset => 0,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_sortby_negative_offset
    create_user_id_table
    records = register_users((0...10).to_a.shuffle)

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  records.sort_by {|id, key| key}[-3..-1],
                  {:table => "user_id",
                   :sortby => "_key",
                   :offset => -3,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_sortby_offset_one_larger_than_hits
    create_user_id_table
    records = register_users((0...10).to_a.shuffle)

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  [],
                  {:table => "user_id",
                   :sortby => "_key",
                   :offset => records.size + 1,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_sortby_negative_offset_one_larger_than_hits
    create_user_id_table
    records = register_users((0...10).to_a.shuffle)

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  [],
                  {:table => "user_id",
                   :sortby => "_key",
                   :offset => -(records.size + 1),
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_sortby_offset_equal_to_hits
    create_user_id_table
    records = register_users((0...10).to_a.shuffle)

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  [],
                  {:table => "user_id",
                   :sortby => "_key",
                   :offset => records.size,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_sortby_negative_offset_equal_to_hits
    create_user_id_table
    records = register_users((0...10).to_a.shuffle)

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  records.sort_by {|id, key| key},
                  {:table => "user_id",
                   :sortby => "_key",
                   :offset => -records.size,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_sortby_limit
    create_user_id_table
    records = register_users((0...10).to_a.shuffle)

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  records.sort_by {|id, key| key}[0, 4],
                  {:table => "user_id",
                   :sortby => "_key",
                   :limit => 4,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_sortby_zero_limit
    create_user_id_table
    records = register_users((0...10).to_a.shuffle)

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  [],
                  {:table => "user_id",
                   :sortby => "_key",
                   :limit => 0,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_sortby_negative_limit
    create_user_id_table
    records = register_users((0...10).to_a.shuffle)

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  records.sort_by {|id, key| key},
                  {:table => "user_id",
                   :sortby => "_key",
                   :limit => -1,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_sortby_offset_and_limit
    create_user_id_table
    records = register_users((0...10).to_a.shuffle)

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  records.sort_by {|id, key| key}[3, 4],
                  {:table => "user_id",
                   :sortby => "_key",
                   :offset => 3,
                   :limit => 4,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_sortby_with_invalid_key
    create_user_id_table
    records = register_users((0...10).to_a.shuffle)

    response = get(command_path(:select,
                                :table => "user_id",
                                :sortby => "-_id,+_-key",
                                :output_type => output_type))
    assert_error_response(Result::INVALID_ARGUMENT,
                          "invalid sort key: <_-key>(<-_id,+_-key>)",
                          response,
                          :content_type => content_type)
  end

  def test_offset
    create_user_id_table
    records = register_users

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  records[3..-1],
                  {:table => "user_id",
                   :offset => 3,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_zero_offset
    create_user_id_table
    records = register_users

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  records,
                  {:table => "user_id",
                   :offset => 0,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_negative_offset
    create_user_id_table
    records = register_users

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  records[-3..-1],
                  {:table => "user_id",
                   :offset => -3,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_offset_one_larger_than_hits
    create_user_id_table
    records = register_users

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  [],
                  {:table => "user_id",
                   :offset => records.size + 1,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_negative_offset_one_larger_than_hits
    create_user_id_table
    records = register_users

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  [],
                  {:table => "user_id",
                   :offset => -(records.size + 1),
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_offset_equal_to_hits
    create_user_id_table
    records = register_users

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  [],
                  {:table => "user_id",
                   :offset => records.size,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_negative_offset_equal_to_hits
    create_user_id_table
    records = register_users

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  records,
                  {:table => "user_id",
                   :offset => -records.size,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_limit
    create_user_id_table
    records = register_users

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  records[0, 4],
                  {:table => "user_id",
                   :limit => 4,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_zero_limit
    create_user_id_table
    records = register_users

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  [],
                  {:table => "user_id",
                   :limit => 0,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_negative_limit
    create_user_id_table
    records = register_users

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  records,
                  {:table => "user_id",
                   :limit => -1,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_offset_and_limit
    create_user_id_table
    records = register_users

    assert_select([["_id", "UInt32"],
                   ["_key", "Int32"]],
                  records[3, 4],
                  {:table => "user_id",
                   :offset => 3,
                   :limit => 4,
                   :output_type => output_type},
                  {:n_hits => records.size,
                   :content_type => content_type})
  end

  def test_accessor
    create_users_table
    load_users
    create_comments_table
    load_comments

    assert_select([["_id", "UInt32"],
                   ["author", "users"],
                   ["text", "ShortText"]],
                  [[2, "hayamiz", "groonga rocks"]],
                  {:table => "comments",
                   :query => "author.real_name:\"Yuto Hayamizu\"",
                   :output_type => output_type},
                  :content_type => content_type)
  end

  def test_drilldown
    create_users_table
    load_users
    create_comments_table
    load_comments

    assert_select([["_id", "UInt32"],
                   ["author", "users"],
                   ["text", "ShortText"]],
                  [[1, "ryoqun", "Ruby rocks"],
                   [2, "hayamiz", "groonga rocks"]],
                  {:table => "comments",
                   :drilldown => "author",
                   :drilldown_output_columns => "real_name",
                   :drilldown_limit => 10,
                   :output_type => output_type},
                  {:drilldown_results => [[[2],
                                           [["real_name", "ShortText"]],
                                           ["Ryo Onodera"],
                                           ["Yuto Hayamizu"]]],
                   :content_type => content_type})
  end

  def test_multiple_drilldown
    create_users_table
    load_users
    create_comments_table
    load_comments

    assert_select([["_id", "UInt32"],
                   ["author", "users"],
                   ["text", "ShortText"]],
                  [[1, "ryoqun", "Ruby rocks"],
                   [2, "hayamiz", "groonga rocks"]],
                  {:table => "comments",
                   :drilldown => "text author",
                   :drilldown_output_columns => "_key",
                   :drilldown_limit => 10,
                   :output_type => output_type},
                  {:drilldown_results => [[[2],
                                           [["_key", "ShortText"]],
                                           ["Ruby rocks"],
                                           ["groonga rocks"]],
                                          [[2],
                                           [["_key", "ShortText"]],
                                           ["ryoqun"],
                                           ["hayamiz"]]],
                   :content_type => content_type})
  end

  def test_drilldown_sortby
    create_users_table
    load_many_users
    create_comments_table
    load_many_comments

    assert_drilldown([["_key", "ShortText"]],
                     [["gunyara-kun"],
                      ["hayamiz"],
                      ["moritan"],
                      ["ryoqun"],
                      ["taporobo"]],
                     {:drilldown_sortby => "_key",
                      :drilldown_output_columns => "_key",
                      :output_type => output_type},
                     :content_type => content_type)
  end

  def test_drilldown_sortby_with_multiple_column
    create_users_table
    load_many_users
    create_comments_table
    load_many_comments

    assert_drilldown([["hp", "Int32"],
                      ["_key", "ShortText"]],
                     [[100, "moritan"],
                      [100, "taporobo"],
                      [150, "gunyara-kun"],
                      [200, "hayamiz"],
                      [200, "ryoqun"]],
                     {:drilldown_sortby => "hp _key",
                      :drilldown_output_columns => "hp _key",
                      :output_type => output_type},
                     :content_type => content_type)
  end

  def test_drilldown_offset
    create_users_table
    load_many_users
    create_comments_table
    comments = load_many_comments

    assert_drilldown([["_key", "ShortText"]],
                     [["gunyara-kun"],
                      ["moritan"],
                      ["ryoqun"]],
                     {:drilldown_offset => 2,
                      :drilldown_output_columns => "_key",
                      :output_type => output_type},
                     {:n_hits => comments.size,
                      :content_type => content_type})
  end

  def test_zero_drilldown_offset
    create_users_table
    load_many_users
    create_comments_table
    load_many_comments

    assert_drilldown([["_key", "ShortText"]],
                     [["taporobo"],
                      ["hayamiz"],
                      ["gunyara-kun"],
                      ["moritan"],
                      ["ryoqun"]],
                     {:drilldown_offset => 0,
                      :drilldown_output_columns => "_key",
                      :output_type => output_type},
                     :content_type => content_type)
  end

  def test_negative_drilldown_offset
    create_users_table
    load_many_users
    create_comments_table
    comments = load_many_comments

    assert_drilldown([["_key", "ShortText"]],
                     [["moritan"],
                      ["ryoqun"]],
                     {:drilldown_offset => -2,
                      :drilldown_output_columns => "_key",
                      :output_type => output_type},
                     {:n_hits => comments.size,
                      :content_type => content_type})
  end

  def test_drilldown_offset_one_larger_than_hits
    create_users_table
    load_many_users
    create_comments_table
    load_many_comments

    assert_drilldown([["_key", "ShortText"]],
                     [],
                     {
                       :drilldown_output_columns => '_key',
                       :drilldown_offset => 6,
                       :output_type => output_type
                     },
                     {
                       :n_hits => 5,
                       :content_type => content_type
                     })
  end

  def test_negative_drilldown_offset_one_larger_than_hits
    create_users_table
    load_many_users
    create_comments_table
    load_many_comments

    assert_drilldown([["_key", "ShortText"]],
                     [],
                     {
                       :drilldown_output_columns => '_key',
                       :drilldown_offset => -6,
                       :output_type => output_type
                     },
                     {
                       :n_hits => 5,
                       :content_type => content_type
                     })
  end

  def test_drilldown_offset_equal_to_hits
    create_users_table
    load_many_users
    create_comments_table
    comments = load_many_comments

    assert_drilldown([["_key", "ShortText"]],
                     [],
                     {
                       :drilldown_output_columns => '_key',
                       :drilldown_offset => 5,
                       :output_type => output_type
                     },
                     {
                       :n_hits => 5,
                       :content_type => content_type
                     })
  end

  def test_negative_drilldown_offset_equal_to_hits
    create_users_table
    load_many_users
    create_comments_table
    comments = load_many_comments

    assert_drilldown([["_key", "ShortText"]],
                     [
                       ["taporobo"],
                       ["hayamiz"],
                       ["gunyara-kun"],
                       ["moritan"],
                       ["ryoqun"]
                     ],
                     {
                       :drilldown_output_columns => '_key',
                       :drilldown_offset => -5,
                       :output_type => output_type
                     },
                     {
                       :n_hits => 5,
                       :content_type => content_type
                     })
  end

  def test_drilldown_limit
    create_users_table
    load_many_users
    create_comments_table
    comments = load_many_comments

    assert_drilldown([["_key", "ShortText"]],
                     [["taporobo"],
                      ["hayamiz"]],
                     {:drilldown_limit => 2,
                      :drilldown_output_columns => "_key",
                      :output_type => output_type},
                     {:n_hits => comments.size,
                      :content_type => content_type})
  end

  def test_zero_drilldown_limit
    create_users_table
    load_many_users
    create_comments_table
    comments = load_many_comments

    assert_drilldown([["_key", "ShortText"],
                      ["_nsubrecs", "Int32"]],
                     [],
                     {:drilldown_limit => 0,
                      :output_type => output_type},
                     {:n_hits => comments.size,
                      :content_type => content_type})
  end

  def test_negative_drilldown_limit
    create_users_table
    load_many_users
    create_comments_table
    comments = load_many_comments

    assert_drilldown([["_key", "ShortText"]],
                     [["taporobo"],
                      ["hayamiz"],
                      ["gunyara-kun"]],
                     {:drilldown_limit => -3,
                      :drilldown_output_columns => "_key",
                      :output_type => output_type},
                     {:n_hits => comments.size,
                      :content_type => content_type})

  end

  def test_drilldown_offset_and_limit
    create_users_table
    load_many_users
    create_comments_table
    comments = load_many_comments

    assert_drilldown([["_key", "ShortText"]],
                     [["gunyara-kun"]],
                     {:drilldown_offset => 2,
                      :drilldown_limit => 1,
                      :drilldown_output_columns => "_key",
                      :output_type => output_type},
                     {:n_hits => comments.size,
                      :content_type => content_type})
  end

  def test_drilldown_output_columns_wild_card
    omit('* is not supported on drilldown_output_columns.')

    create_users_table
    load_many_users
    create_comments_table
    comments = load_many_comments

    assert_drilldown([["real_name", "ShortText"],
                      ["hp", "Int32"],
                      ["_key", "ShortText"]],
                     [["モリタン", 100, "moritan"],
                      ["タポロボ", 100, "taporobo"],
                      ["Tasuku SUENAGA", 150, "gunyara-kun"],
                      ["Yuto Hayamizu", 200, "hayamiz"],
                      ["Ryo Onodera", 200, "ryoqun"]],
                     {:drilldown_output_columns => "* _key",
                      :output_type => output_type},
                     :content_type => content_type)
  end

  def test_xml
    populate_users

    expected = <<EOF
<?xml version="1.0" encoding="utf-8"?>
<SEGMENTS>
<SEGMENT>
<RESULTPAGE>
<RESULTSET OFFSET="0" LIMIT="2" NHITS="2">
<HIT NO="1">
<FIELD NAME="_id">2</FIELD>
<FIELD NAME="_key">hayamiz</FIELD>
<FIELD NAME="real_name">Yuto Hayamizu</FIELD>
<FIELD NAME="hp">200</FIELD>
</HIT>
<HIT NO="2">
<FIELD NAME="_id">1</FIELD>
<FIELD NAME="_key">ryoqun</FIELD>
<FIELD NAME="real_name">Ryo Onodera</FIELD>
<FIELD NAME="hp">200</FIELD>
</HIT>
</RESULTSET>
</RESULTPAGE>
</SEGMENT>
</SEGMENTS>
EOF
    assert_select_xml(expected,
                      {:table => "users",
                       :output_columns => "_id,_key,real_name,hp"})
  end

  def test_xml_vector
    table_create("Softwares",
                 :flags => Table::HASH_KEY,
                 :key_type => "ShortText")
    column_create("Softwares", "tags", Column::VECTOR, "ShortText")
    column_create("Softwares", "description", Column::SCALAR, "ShortText")
    load("Softwares",
         [[:_key, :tags, :description],
          ["groonga",
           ["full-text search", "C"],
           "full-text search engine"],
          ["Ruby",
           ["language", "script", "fun"],
           "An object oriented script language"],
         ["rroonga",
          [],
          "The Ruby bindings for groonga"]])

    expected = <<EOF
<?xml version="1.0" encoding="utf-8"?>
<SEGMENTS>
<SEGMENT>
<RESULTPAGE>
<RESULTSET OFFSET="0" LIMIT="3" NHITS="3">
<HIT NO="1">
<FIELD NAME="_key">groonga</FIELD>
<FIELD NAME="tags">full-text search, C</FIELD>
<FIELD NAME="description">full-text search engine</FIELD>
</HIT>
<HIT NO="2">
<FIELD NAME="_key">Ruby</FIELD>
<FIELD NAME="tags">language, script, fun</FIELD>
<FIELD NAME="description">An object oriented script language</FIELD>
</HIT>
<HIT NO="3">
<FIELD NAME="_key">rroonga</FIELD>
<FIELD NAME="tags"></FIELD>
<FIELD NAME="description">The Ruby bindings for groonga</FIELD>
</HIT>
</RESULTSET>
</RESULTPAGE>
</SEGMENT>
</SEGMENTS>
EOF
    assert_select_xml(expected,
                      {:table => "Softwares",
                       :output_columns => "_key tags description"})
  end

  def test_xml_with_offset
    create_users_table
    load_many_users
    expected = <<EOF
<?xml version="1.0" encoding="utf-8"?>
<SEGMENTS>
<SEGMENT>
<RESULTPAGE>
<RESULTSET OFFSET="2" LIMIT="1" NHITS="5">
<HIT NO="3">
<FIELD NAME="_id">1</FIELD>
</HIT>
</RESULTSET>
</RESULTPAGE>
</SEGMENT>
</SEGMENTS>
EOF
    assert_select_xml(expected,
                      {:table => "users", :offset => 2, :limit => 1,
                       :output_columns => "_id"})
  end

  def test_xml_with_offset_sortby
    create_users_table
    load_many_users
    expected = <<EOF
<?xml version="1.0" encoding="utf-8"?>
<SEGMENTS>
<SEGMENT>
<RESULTPAGE>
<RESULTSET OFFSET="2" LIMIT="3" NHITS="5">
<HIT NO="3">
<FIELD NAME="_id">1</FIELD>
<FIELD NAME="_key">moritan</FIELD>
<FIELD NAME="real_name">モリタン</FIELD>
<FIELD NAME="hp">100</FIELD>
</HIT>
<HIT NO="4">
<FIELD NAME="_id">3</FIELD>
<FIELD NAME="_key">ryoqun</FIELD>
<FIELD NAME="real_name">Ryo Onodera</FIELD>
<FIELD NAME="hp">200</FIELD>
</HIT>
<HIT NO="5">
<FIELD NAME="_id">2</FIELD>
<FIELD NAME="_key">taporobo</FIELD>
<FIELD NAME="real_name">タポロボ</FIELD>
<FIELD NAME="hp">100</FIELD>
</HIT>
</RESULTSET>
</RESULTPAGE>
</SEGMENT>
</SEGMENTS>
EOF
    assert_select_xml(expected,
                      {:table => "users", :sortby => "_key", :offset => 2,
                       :output_columns => "_id,_key,real_name,hp"})
  end

  def test_no_existent_pat_key
    populate_users

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"],
                   ["real_name", "ShortText"]],
                  [],
                  {:table => "users",
                   :output_columns => "_id,_key,real_name",
                   :query => "_key:ababa",
                   :output_type => output_type},
                  :content_type => content_type)
  end

  def test_no_existent_pat_id
    populate_users

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"],
                   ["real_name", "ShortText"]],
                  [],
                  {:table => "users",
                   :output_columns => "_id,_key,real_name",
                   :query => "_id:1234",
                   :output_type => output_type},
                  :content_type => content_type)
  end

  def test_no_existent_hash_key
    populate_tags

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"]],
                  [],
                  {:table => "tags",
                   :output_columns => "_id,_key",
                   :query => "_key:ababa",
                   :output_type => output_type},
                  :content_type => content_type)
  end

  def test_no_existent_hash_id
    populate_tags

    assert_select([["_id", "UInt32"],
                   ["_key", "ShortText"]],
                  [],
                  {:table => "tags",
                   :output_columns => "_id,_key",
                   :query => "_id:1234",
                   :output_type => output_type},
                  :content_type => content_type)
  end

  private
  def create_user_id_table
    table_create("user_id", :flags => Table::HASH_KEY, :key_type => "Int32")
  end

  def register_users(keys=nil)
    header = ["_key"]
    keys ||= (0...10).to_a

    load("user_id", [header, *keys.collect {|key| [key]}])

    id = 0
    keys.collect do |key|
      id += 1
      [id, key]
    end
  end

  def create_comments_table
    table_create("comments", :flags => Table::NO_KEY)
    column_create("comments", "text", Column::SCALAR, "ShortText")
    column_create("comments", "author", Column::SCALAR, "users")
  end

  def load_comments
    load("comments",
         [[:text, :author],
          ["Ruby rocks", "ryoqun"],
          ["groonga rocks", "hayamiz"]])
  end

  def load_many_comments
    comments = [["Ruby最高！", "taporobo"],
                ["groonga最高！", "hayamiz"],
                ["Ruby/groonga is useful.", "gunyara-kun"],
                ["Ruby rocks!", "moritan"],
                ["groonga rocks!", "ryoqun"]]
    load("comments",
         [[:text, :author],
          *comments])
    comments
  end

  def assert_drilldown(header, expected, parameters, options={})
    assert_select([["_id", "UInt32"],
                   ["author", "users"],
                   ["text", "ShortText"]],
                  [[1, "taporobo", "Ruby最高！"],
                   [2, "hayamiz", "groonga最高！"],
                   [3, "gunyara-kun", "Ruby/groonga is useful."],
                   [4, "moritan", "Ruby rocks!"],
                   [5, "ryoqun", "groonga rocks!"]],
                  {:table => "comments",
                   :drilldown => "author",
                   :drilldown_limit => 10}.merge(parameters),
                  {:drilldown_results => [[[options[:n_hits] || expected.size],
                                           header,
                                           *expected]],
                   :content_type => options[:content_type]})
  end
end

module DefineSelector
  def assert_select(header, expected, parameters, options={}, &block)
    @names ||= []
    name = "custom_select#{@names.size}"
    @names << name
    response = get(command_path("define_selector",
                                parameters.merge(:name => name)))
    assert_success_response(response, :content_type => content_type)

    execution_parameters = {}
    if parameters[:output_type]
      execution_parameters[:output_type] = output_type
    end
    super(header,
          expected,
          execution_parameters,
          options.merge(:command => name),
          &block)
  end
end

class JSONHTTPSelectBasicTest < Test::Unit::TestCase
  include HTTPSelectBasicTests
  include Format::JSON
end

class JSONHTTPDefineSelectorBasicTest < Test::Unit::TestCase
  include HTTPSelectBasicTests
  include Format::JSON
  include DefineSelector
end

class MessagePackHTTPSelectBasicTest < Test::Unit::TestCase
  include HTTPSelectBasicTests
  include Format::MessagePack
end

class MessagePackHTTPDefineSelectorBasicTest < Test::Unit::TestCase
  include HTTPSelectBasicTests
  include Format::MessagePack
  include DefineSelector
end
