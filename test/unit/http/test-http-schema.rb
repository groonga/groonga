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

class HTTPSchemaTest < Test::Unit::TestCase
  include GroongaHTTPTestUtils

  def setup
    setup_server
  end

  def teardown
    teardown_server
  end

  def test_table_list_empty
    response = get(command_path(:table_list))
    assert_response([["id", "name", "path", "flags", "domain"]],
                    response,
                    :content_type => "application/json")
  end

  def test_table_list_exist
    create_bookmarks_table
    response = get(command_path(:table_list))
    assert_response([
                     ["id", "name", "path", "flags", "domain"],
                     [@bookmarks_table_id,
                      "bookmarks",
                      nil,
                      Flag::PERSISTENT | Table::PAT_KEY | Key::VAR_SIZE,
                      Type::SHORT_TEXT],
                    ],
                    response,
                    :content_type => "application/json") do |actual|
      actual[0, 1] + actual[1..-1].collect do |values|
        id, name, path, flags, domain = values
        [id, name, nil, flags, domain]
      end
    end
  end

  def test_table_list_with_invalid_output_type
    response = get(command_path(:table_list,
                                :output_type => "unknown"))
    pend("should implement error case") do
      assert_response([[Result::UNKNOWN_ERROR, "should be implemented"]],
                      response,
                      :content_type => "application/json")
    end
  end

  def test_column_list_empty
    create_bookmarks_table
    response = get(command_path(:column_list,
                                :table => "bookmarks"))
    assert_response([["id", "name", "path", "type", "flags", "domain"]],
                    response,
                    :content_type => "application/json")
  end

  def test_column_list_exist
    create_bookmarks_table
    create_bookmark_title_column
    response = get(command_path(:column_list,
                                :table => "bookmarks"))
    assert_response([
                     ["id", "name", "path", "type", "flags", "domain"],
                     [@bookmarks_title_column_id,
                      "title",
                      nil,
                      "var",
                      Column::SCALAR | Flag::PERSISTENT | Key::VAR_SIZE,
                      @bookmarks_table_id]
                    ],
                    response,
                    :content_type => "application/json") do |actual|
      actual[0, 1] + actual[1..-1].collect do |values|
        id, name, path, type, flags, domain = values
        [id, name, nil, type, flags, domain]
      end
    end
  end

  def test_column_list_nonexistent
    response = get(command_path(:column_list,
                                :table => "nonexistent"))
    pend("should implement error case") do
      assert_response([[Result::UNKNOWN_ERROR, :message]],
                      response,
                      :content_type => "application/json")
    end
  end

  def test_column_list_without_table
    response = get(command_path(:column_list))
    pend("should implement error case") do
      assert_response([[Result::UNKNOWN_ERROR, "should be implemented"]],
                      response,
                      :content_type => "application/json")
    end
  end

  def test_column_list_with_invalid_output_type
    create_bookmarks_table
    response = get(command_path(:column_list,
                                :table => "bookmarks",
                                :output_type => "unknown"))
    pend("should implement error case") do
      assert_response([[Result::UNKNOWN_ERROR, "should be implemented"]],
                      response,
                      :content_type => "application/json")
    end
  end

  def test_column_list_with_invalid_output_type_without_table
    response = get(command_path(:column_list,
                                :output_type => "unknown"))
    pend("should implement error case") do
      assert_response([[Result::UNKNOWN_ERROR, "should be implemented"]],
                      response,
                      :content_type => "application/json")
    end
  end

  def test_full_text_search
    create_bookmarks_table
    create_bookmark_title_column

    response = get(command_path(:table_create,
                                :name => "terms",
                                :flags => Table::PAT_KEY | Key::NORMALIZE,
                                :key_type => "ShortText",
                                :default_tokenizer => "TokenBigram"))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "application/json")

    response = get(command_path(:column_create,
                                :table => "terms",
                                :name => "bookmarks-title",
                                :flags => Column::INDEX | Flag::WITH_POSITION,
                                :type => "bookmarks",
                                :source => "title"))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "application/json")

    groonga_title = "groonga - an open-source fulltext search engine " +
                    "and column store."
    senna_title = "Senna: An Embeddable Fulltext Search Engine"
    load("bookmarks",
         [{"_key" => "groonga", "title" => groonga_title},
          {"_key" => "senna", "title" => senna_title}])

    assert_select(["_key", "title"],
                  [["groonga", groonga_title]],
                  :table => "bookmarks",
                  :output_columns => "_key title",
                  :query => "title:@column")
  end

  private
  def create_bookmarks_table
    response = get(command_path(:table_create,
                                :name => "bookmarks",
                                :flags => Table::PAT_KEY,
                                :key_type => "ShortText",
                                :value_type => "Object",
                                :default_tokenizer => ""))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "application/json")
    @bookmarks_table_id = object_registered
  end

  def create_bookmark_title_column
    response = get(command_path(:column_create,
                                :table => "bookmarks",
                                :name => "title",
                                :flags => Column::SCALAR,
                                :type => "ShortText"))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "application/json")
    @bookmarks_title_column_id = object_registered
  end
end
