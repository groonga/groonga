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

module HTTPSelectDrilldownTests
  include GroongaHTTPTestUtils

  def setup
    setup_server
  end

  def teardown
    teardown_server
  end

  def test_zero_limit
    table_create("Initial", :flags => Table::PAT_KEY, :key_type => "ShortText")
    table_create("Person", :key_type => "ShortText")
    column_create("Person", "initial", Column::SCALAR, "Initial")
    column_create("Person", "kana", Column::SCALAR, "ShortText")
    table_create("Event", :key_type => "ShortText")
    column_create("Event", "date", Column::SCALAR, "Time")
    column_create("Event", "person", Column::VECTOR, "Person")
    column_create("Event", "title", Column::SCALAR, "ShortText")
    column_create("Event", "search", Column::SCALAR, "ShortText")
    table_create("Bigram",
                 :flags => Table::PAT_KEY | Key::NORMALIZE,
                 :key_type => "ShortText",
                 :default_tokenizer => "TokenBigram")
    column_create("Bigram", "index", Column::INDEX | Flag::WITH_POSITION,
                  "Event", :source => "search")

    add_user("グニャラくん", "ぐにゃらくん", "か")
    add_user("morita", "もりた", "ま")
    add_user("yu", "ゆう", "や")
    add_event("グニャラくん", nil, "groongaリリース（前編）", "20091218")
    add_event("グニャラくん", nil, "groongaリリース（後編）", "20091218")
    add_event("morita", nil, "groonga（ぐるんが）解説・パート1", "20091218")
    add_event("yu", nil, "groonga（ぐるんが）解説・パート2", "20091219")

    assert_drilldown(["_key"],
                     [],
                     [[[2],
                       ["_key", "_nsubrecs", "initial"],
                       ["グニャラくん", 2, "か"],
                       ["morita", 1, "ま"]]],
                     {
                       :table => "Event",
                       :match_column => "search",
                       :query => "groonga",
                       :filter => "date == \"20091218\" &&  person @ \"グニ\"",
                       :limit => 0,
                       :output_columns => "_key",
                       :drilldown => "person",
                       :drilldown_sortby => "kana",
                       :drilldown_output_columns => "_key _nsubrecs initial",
                       :drilldown_offset => 0,
                       :drilldown_limit => -1,
                     },
                     {:n_hits => 3})
  end

  def test_many_columns
    table_create("Initial", :flags => Table::PAT_KEY, :key_type => "ShortText")
    table_create("Person", :key_type => "ShortText")
    column_create("Person", "initial", Column::SCALAR, "Initial")
    column_create("Person", "kana", Column::SCALAR, "ShortText")
    table_create("Place", :key_type => "ShortText")
    column_create("Place", "name", Column::SCALAR, "ShortText")
    table_create("Event", :key_type => "ShortText")
    column_create("Event", "date", Column::SCALAR, "Time")
    column_create("Event", "person", Column::VECTOR, "Person")
    column_create("Event", "place", Column::SCALAR, "Place")
    column_create("Event", "title", Column::SCALAR, "ShortText")
    column_create("Event", "search", Column::SCALAR, "ShortText")
    table_create("Bigram",
                 :flags => Table::PAT_KEY | Key::NORMALIZE,
                 :key_type => "ShortText",
                 :default_tokenizer => "TokenBigram")
    column_create("Bigram", "index", Column::INDEX | Flag::WITH_POSITION,
                  "Event", :source => "search")

    add_user("グニャラくん", "ぐにゃらくん", "か")
    add_user("morita", "もりた", "ま")
    add_user("yu", "ゆう", "や")
    add_place("razil.jp", "ブラジル")
    add_place("shinjuku", "新宿")
    add_event("グニャラくん", "razil.jp", "groongaリリース（前編）", "20091218")
    add_event("グニャラくん", "shinjuku", "groongaリリース（後編）", "20091218")
    add_event("morita", "razil.jp",
              "groonga（ぐるんが）解説・パート1", "20091218")
    add_event("yu", "shinjuku", "groonga（ぐるんが）解説・パート2", "20091219")

    assert_drilldown(["_key", "place", "place.name",
                      "title", "person", "date"],
                     [["3", "shinjuku", "新宿",
                       "groonga（ぐるんが）解説・パート2", ["yu"], 20091219.0],
                      ["0", "razil.jp", "ブラジル", "groongaリリース（前編）",
                       ["グニャラくん"], 20091218.0]],
                     [[[2],
                       ["_key", "_nsubrecs"],
                       [20091218.0, 3],
                       [20091219.0, 1]],
                      [[3],
                       ["_key", "_nsubrecs"],
                       ["グニャラくん", 2],
                       ["yu", 1],
                       ["morita", 1]],
                      [[2],
                       ["_key", "_nsubrecs", "name"],
                       ["razil.jp", 2, "ブラジル"],
                       ["shinjuku", 2, "新宿"]]],
                     {
                       :table => "Event",
                       :match_column => "search",
                       :query => "groonga",
                       :filter => "",
                       :sortby => "-place._key",
                       :offset => 1,
                       :limit => 2,
                       :output_columns => ["_key", "place", "place.name",
                                           "title", "person", "date"].join(" "),
                       :drilldown => "date person place",
                       :drilldown_sortby => "-_nsubrecs",
                       :drilldown_output_columns => "_key _nsubrecs name",
                       :drilldown_limit => 3,
                     },
                     {:n_hits => 4})
  end

  private
  def add_user(name, kana, initial)
    load("Initial", [{"_key" => initial}])
    load("Person", [{"_key" => name, "kana" => kana, "initial" => initial}])
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
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "application/json")
    super(header, expected, {}, options.merge(:command => name), &block)
  end
end
