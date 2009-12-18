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

  def test_no_limit
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
    add_event("グニャラくん", "groongaリリース（前編）", "20091218")
    add_event("グニャラくん", "groongaリリース（後編）", "20091218")
    add_event("morita", "groonga（ぐるんが）解説・パート1", "20091218")
    add_event("yu", "groonga（ぐるんが）解説・パート2", "20091219")

    assert_drilldown(["_key"],
                     [],
                     ["_key", "_nsubrecs", "initial"],
                     [["グニャラくん", 2, "か"],
                      ["morita", 1, "ま"]],
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

  private
  def add_user(name, kana, initial)
    load("Initial", [{"_key" => initial}])
    load("Person", [{"_key" => name, "kana" => kana, "initial" => initial}])
  end

  def add_event(person, title, date)
    @event_id ||= 0
    load("Event", [{"_key" => @event_id,
                    "title" => title,
                    "search" => title,
                    "date" => date,
                    "person" => person}])
    @event_id += 1
  end

  def assert_drilldown(header, expected, drilldown_header, drilldown_records,
                       parameters, options={})
    assert_select(header,
                  expected,
                  parameters,
                  options.merge(:drilldown_results => [[[drilldown_records.size],
                                                        drilldown_header,
                                                        *drilldown_records]]))
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
