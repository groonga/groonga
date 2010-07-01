# -*- coding: utf-8 -*-
#
# Copyright (C) 2009-2010  Kouhei Sutou <kou@clear-code.com>
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

module HTTPSelectDrilldownTests
  include GroongaHTTPTestUtils

  def setup
    setup_server
    setup_ddl
    setup_data
  end

  def setup_ddl
    table_create("Initial", :flags => Table::PAT_KEY, :key_type => "ShortText")
    table_create("Tag", :flags => Table::HASH_KEY | Key::NORMALIZE,
                 :key_type => "ShortText")
    table_create("Person", :key_type => "ShortText")
    column_create("Person", "initial", Column::SCALAR, "Initial")
    column_create("Person", "kana", Column::SCALAR, "ShortText")
    column_create("Person", "tags", Column::VECTOR, "Tag")
    table_create("Place", :key_type => "ShortText")
    column_create("Place", "name", Column::SCALAR, "ShortText")
    table_create("Event", :key_type => "ShortText")
    column_create("Event", "person", Column::VECTOR, "Person")
    column_create("Event", "place", Column::SCALAR, "Place")
    column_create("Event", "title", Column::SCALAR, "ShortText")
    column_create("Event", "search", Column::SCALAR, "ShortText")
    column_create("Event", "date", Column::SCALAR, "Time")
    table_create("Bigram",
                 :flags => Table::PAT_KEY | Key::NORMALIZE,
                 :key_type => "ShortText",
                 :default_tokenizer => "TokenBigram")
    column_create("Bigram", "index", Column::INDEX | Flag::WITH_POSITION,
                  "Event", :source => "search")
  end

  def setup_data
    add_user("グニャラくん", "ぐにゃらくん", "く", ["ごくつぶし"])
    add_user("morita", "もりた", "も", ["役員", "プログラマ", "管理職"])
    add_user("yu", "ゆう", "ゆ", ["サーバ管理"])
    add_place("razil.jp", "ブラジル")
    add_place("shinjuku", "新宿")
    add_event("グニャラくん", "razil.jp", "groongaリリース（前編）", "20091218")
    add_event("グニャラくん", "shinjuku", "groongaリリース（後編）", "20091218")
    add_event("morita", "razil.jp",
              "groonga（ぐるんが）解説・パート1", "20091218")
    add_event("yu", "shinjuku", "groonga（ぐるんが）解説・パート2", "20091219")
    add_event("yu", "shinjuku", "groonga（ぐるんが）解説・パート3", "20091220")
    add_event("yu", "shinjuku", "groonga（ぐるんが）解説・パート4", "20091220")

    add_event("morita", "razil.jp", "肉の会・パート1", "20091221")
    add_event("morita", "razil.jp", "肉の会・パート2", "20091222")
    add_event("morita", "razil.jp", "肉の会・パート3", "20091223")
    add_event("morita", "razil.jp", "肉の会・パート4", "20091224")
    add_event("morita", "razil.jp", "肉の会・パート5", "20091225")
    add_event("morita", "razil.jp", "肉の会・パート6", "20091226")
    add_event("morita", "razil.jp", "肉の会・パート7", "20091227")
    add_event("morita", "razil.jp", "肉の会・パート8", "20091228")
    add_event("morita", "razil.jp", "肉の会・パート9", "20091229")
  end

  def teardown
    teardown_server
  end

  def test_zero_limit
    assert_drilldown([["_key", "ShortText"]],
                     [],
                     [[[2],
                       [["_key", "ShortText"],
                        ["_nsubrecs", "Int32"],
                        ["initial", "Initial"]],
                       ["グニャラくん", 2, "く"],
                       ["morita", 1, "も"]]],
                     {
                       :table => "Event",
                       :match_columns => "search",
                       :query => "groonga",
                       :filter => "date == \"20091218\"",
                       :limit => 0,
                       :output_columns => "_key",
                       :drilldown => "person",
                       :drilldown_sortby => "kana",
                       :drilldown_output_columns => "_key,_nsubrecs,initial",
                       :drilldown_offset => 0,
                       :drilldown_limit => -1,
                     },
                     {:n_hits => 3})
  end

  def test_many_columns
    assert_drilldown([["_key", "ShortText"],
                      ["place", "Place"],
                      ["place.name", "ShortText"],
                      ["title", "ShortText"],
                      ["person", "Person"],
                      ["date", "Time"]],
                     [["4", "shinjuku", "新宿",
                       "groonga（ぐるんが）解説・パート3", ["yu"], 20091220.0],
                      ["3", "shinjuku", "新宿",
                       "groonga（ぐるんが）解説・パート2", ["yu"], 20091219.0]],
                     [[[3],
                       [["_key", "Time"],
                        ["_nsubrecs", "Int32"]],
                       [20091218.0, 3],
                       [20091220.0, 2],
                       [20091219.0, 1]],
                      [[3],
                       [["_key", "ShortText"],
                        ["_nsubrecs", "Int32"]],
                       ["yu", 3],
                       ["グニャラくん", 2],
                       ["morita", 1]],
                      [[2],
                       [["_key", "ShortText"],
                        ["_nsubrecs", "Int32"],
                        ["name", "ShortText"]],
                       ["shinjuku", 4, "新宿"],
                       ["razil.jp", 2, "ブラジル"]]],
                     {
                       :table => "Event",
                       :match_columns => "search",
                       :query => "groonga",
                       :filter => "",
                       :sortby => "-place._key",
                       :offset => 1,
                       :limit => 2,
                       :output_columns => ["_key", "place", "place.name",
                                           "title", "person", "date"].join(" "),
                       :drilldown => "date person place",
                       :drilldown_sortby => "-_nsubrecs",
                       :drilldown_output_columns => "_key,_nsubrecs,name",
                       :drilldown_limit => 3,
                     },
                     {:n_hits => 6})
  end

  def test_default_output_columns
    assert_drilldown([["title", "ShortText"],
                      ["person", "Person"]],
                     [["groongaリリース（前編）", ["グニャラくん"]],
                      ["groongaリリース（後編）", ["グニャラくん"]]],
                     [[[3],
                       [["_key", "ShortText"],
                        ["_nsubrecs", "Int32"]],
                       ["yu", 3],
                       ["グニャラくん", 2]]],
                     {
                       :table => "Event",
                       :match_columns => "search",
                       :query => "groonga",
                       :sortby => "title",
                       :limit => 2,
                       :output_columns => "title,person",
                       :drilldown => "person",
                       :drilldown_sortby => "-_nsubrecs",
                       :drilldown_limit => 2,
                     },
                     {:n_hits => 6})
  end

  def test_default_limit
    assert_drilldown([["title", "ShortText"],
                      ["date", "Time"]],
                     [["肉の会・パート1", 20091221.0]],
                     [[[12],
                       [["_key", "Time"],
                        ["_nsubrecs", "Int32"]],
                       [20091229.0, 1],
                       [20091228.0, 1],
                       [20091227.0, 1],
                       [20091226.0, 1],
                       [20091225.0, 1],
                       [20091224.0, 1],
                       [20091223.0, 1],
                       [20091222.0, 1],
                       [20091221.0, 1],
                       [20091220.0, 2]]],
                     {
                       :table => "Event",
                       :sortby => "title",
                       :limit => 1,
                       :output_columns => "title date",
                       :drilldown => "date",
                       :drilldown_sortby => "-_key",
                       :drilldown_output_columns => "_key,_nsubrecs",
                     },
                     {:n_hits => 15})
  end

  def test_no_limit
    assert_drilldown([["_key", "ShortText"]],
                     [["morita"]],
                     [[[3],
                       [["_key", "ShortText"],
                        ["_nsubrecs", "Int32"]],
                       ["管理職", 1],
                       ["役員", 1],
                       ["プログラマ", 1]]],
                     {
                       :table => "Person",
                       :query => "_key:morita",
                       :output_columns => "_key",
                       :drilldown => "tags",
                       :drilldown_sortby => "-_key",
                       :drilldown_limit => -1,
                       :drilldown_output_columns => "_key,_nsubrecs",
                     },
                     {:n_hits => 1})
  end

  private
  def add_user(name, kana, initial, tags)
    load("Initial", [{"_key" => initial}])
    tags.each{|tag|
      load("Tag", [{"_key" => tag}])
    }
    load("Person", [{"_key" => name, "kana" => kana, "initial" => initial, "tags" => tags}])
  end

  def add_place(key, name)
    load("Place", [{"_key" => key, "name" => name}])
  end

  def add_event(person, place, title, date)
    @event_id ||= 0
    attributes = {
      "_key" => @event_id,
      "title" => title,
      "search" => title,
      "date" => date,
      "person" => person,
    }
    attributes["place"] = place if place
    load("Event", [attributes])
    @event_id += 1
  end

  def assert_drilldown(header, expected, drilldown_results,
                       parameters, options={})
    assert_select(header,
                  expected,
                  parameters,
                  options.merge(:drilldown_results => drilldown_results))
  end
end

class HTTPSelectDrilldownTest < Test::Unit::TestCase
  include HTTPSelectDrilldownTests
end

class HTTPDefineSelectorDrilldownTest < HTTPSelectDrilldownTest
  include HTTPSelectDrilldownTests

  def assert_select(header, expected, parameters, options={}, &block)
    name = "custom_select"
    response = get(command_path("define_selector",
                                parameters.merge(:name => name)))
    assert_response([success_status_response], response,
                    :content_type => "application/json")
    super(header, expected, {}, options.merge(:command => name), &block)
  end
end
