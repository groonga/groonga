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
  module Utils
    include GroongaHTTPTestUtils

    def setup
      setup_server
    end

    def teardown
      teardown_server
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

    def assert_table_list(expected)
      response = get(command_path(:table_list))
      expected = expected.collect do |values|
        name, flags, domain = values
        [nil, name, nil, flags, domain]
      end
      assert_response([
                       ["id", "name", "path", "flags", "domain"],
                       *expected
                      ],
                      response,
                      :content_type => "application/json") do |actual|
        actual[0, 1] + actual[1..-1].collect do |values|
          id, name, path, flags, domain = values
          [nil, name, nil, flags, domain]
        end
      end
    end
  end

  include Utils

  def test_table_list_empty
    response = get(command_path(:table_list))
    assert_response([["id", "name", "path", "flags", "domain"]],
                    response,
                    :content_type => "application/json")
  end

  def test_table_list_exist
    create_bookmarks_table

    response = get(command_path(:table_list))
    normalized_path = "/path/to/table"
    assert_response([
                     ["id", "name", "path", "flags", "domain"],
                     [@bookmarks_table_id,
                      "bookmarks",
                      normalized_path,
                      Flag::PERSISTENT | Table::PAT_KEY | Key::VAR_SIZE,
                      Type::SHORT_TEXT],
                    ],
                    response,
                    :content_type => "application/json") do |actual|
      actual[0, 1] + actual[1..-1].collect do |values|
        id, name, path, flags, domain = values
        path = normalized_path if path
        [id, name, path, flags, domain]
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

  def test_table_create_without_name
    response = get(command_path(:table_create))
    assert_response([[Result::UNKNOWN_ERROR,
                      "should not create anonymous table"]],
                    response,
                    :content_type => "application/json")
  end

  def test_table_create_with_dot_name
    response = get(command_path(:table_create, :name => "mori.daijiro"))
    assert_response([[Result::INVALID_ARGUMENT, "name contains '.'"]],
                    response,
                    :content_type => "application/json")
  end

  def test_table_create_with_under_score_started_name
    response = get(command_path(:table_create, :name => "_mori"))
    assert_response([[Result::INVALID_ARGUMENT, "name starts with '_'"]],
                    response,
                    :content_type => "application/json")
  end

  def test_table_create_with_under_score_name
    response = get(command_path(:table_create, :name => "mori_daijiro"))
    assert_response([[Result::SUCCESS]],
                    response,
                    :content_type => "application/json")
  end

  def test_table_create_with_colon_name
    response = get(command_path(:table_create, :name => "daijiro:mori"))
    assert_response([[Result::INVALID_ARGUMENT, "name contains ':'"]],
                    response,
                    :content_type => "application/json")
  end

  def test_table_create_with_duplicated_name
    response = get(command_path(:table_create, :name => "table_create"))
    assert_response([[Result::INVALID_ARGUMENT,
                      "already used name was assigned"]],
                    response,
                    :content_type => "application/json")
  end

  def test_table_create_with_duplicated_name
    response = get(command_path(:table_create, :name => "table_create"))
    assert_response([[Result::INVALID_ARGUMENT,
                      "already used name was assigned"]],
                    response,
                    :content_type => "application/json")
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

  class HashCreateTest < Test::Unit::TestCase
    include Utils

    def test_simple
      response = get(command_path(:table_create, :name => "users"))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")

      assert_table_list([["users",
                          Flag::PERSISTENT | Table::HASH_KEY,
                          Type::VOID]])
    end

    def test_normalize_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Key::NORMALIZE))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")

      assert_table_list([["users",
                          Flag::PERSISTENT | Table::HASH_KEY | Key::NORMALIZE,
                          Type::VOID]])
    end

    def test_normalized_string_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Key::NORMALIZE,
                                  :key_type => "ShortText"))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")

      assert_table_list([["users",
                          Flag::PERSISTENT | Table::HASH_KEY |
                          Key::NORMALIZE | Key::VAR_SIZE,
                          Type::SHORT_TEXT]])
    end

    def test_view_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")
      users_table_id = object_registered

      response = get(command_path(:table_create,
                                  :name => "sites",
                                  :key_type => "users"))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")

      assert_table_list([["sites",
                          Flag::PERSISTENT | Table::HASH_KEY,
                          users_table_id],
                         ["users",
                          Flag::PERSISTENT | Table::VIEW,
                          Type::VOID]])
    end

    def test_long_size_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :key_type => "Text"))
      assert_response([[Result::UNKNOWN_ERROR, "should implement error case"]],
                      response,
                      :content_type => "application/json")

      assert_table_list([])
    end

    def test_sis
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Key::SIS,
                                  :key_type => "ShortText"))
      assert_response([[Result::UNKNOWN_ERROR, "SIS is invalid flag for hash"]],
                      response,
                      :content_type => "application/json")

      assert_table_list([])
    end

    def test_nonexistent_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :key_type => "nonexistent"))
      assert_response([[Result::UNKNOWN_ERROR, "should implement error case"]],
                      response,
                      :content_type => "application/json")

      assert_table_list([])
    end

    def test_invalid_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :key_type => "table_create"))
      assert_response([[Result::UNKNOWN_ERROR, "should implement error case"]],
                      response,
                      :content_type => "application/json")

      assert_table_list([])
    end

    def test_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :value_type => "Int32"))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")

      assert_table_list([["users",
                          Flag::PERSISTENT | Table::HASH_KEY,
                          Type::VOID]])
    end

    def test_nonexistent_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :value_type => "nonexistent"))
      assert_response([[Result::UNKNOWN_ERROR, "should implement error case"]],
                      response,
                      :content_type => "application/json")

      assert_table_list([])
    end
  end

  class PatriciaTrieCreateTest < Test::Unit::TestCase
    include Utils

    def test_simple
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")

      assert_table_list([["users",
                          Flag::PERSISTENT | Table::PAT_KEY,
                          Type::VOID]])
    end

    def test_normalize_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY | Key::NORMALIZE))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")

      assert_table_list([["users",
                          Flag::PERSISTENT | Table::PAT_KEY | Key::NORMALIZE,
                          Type::VOID]])
    end

    def test_normalized_string_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY | Key::NORMALIZE,
                                  :key_type => "ShortText"))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")

      assert_table_list([["users",
                          Flag::PERSISTENT | Table::PAT_KEY |
                          Key::NORMALIZE | Key::VAR_SIZE,
                          Type::SHORT_TEXT]])
    end

    def test_view_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")
      users_table_id = object_registered

      response = get(command_path(:table_create,
                                  :name => "sites",
                                  :flags => Table::PAT_KEY,
                                  :key_type => "users"))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")

      assert_table_list([["sites",
                          Flag::PERSISTENT | Table::PAT_KEY,
                          users_table_id],
                         ["users",
                          Flag::PERSISTENT | Table::VIEW,
                          Type::VOID]])
    end

    def test_long_size_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY,
                                  :key_type => "Text"))
      assert_response([[Result::UNKNOWN_ERROR, "should implement error case"]],
                      response,
                      :content_type => "application/json")

      assert_table_list([])
    end

    def test_sis
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY | Key::SIS,
                                  :key_type => "ShortText"))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")

      assert_table_list([["users",
                          Flag::PERSISTENT | Table::PAT_KEY |
                          Key::VAR_SIZE | Key::SIS,
                          Type::SHORT_TEXT]])
    end

    def test_nonexistent_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY,
                                  :key_type => "nonexistent"))
      assert_response([[Result::UNKNOWN_ERROR, "should implement error case"]],
                      response,
                      :content_type => "application/json")

      assert_table_list([])
    end

    def test_invalid_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY,
                                  :key_type => "table_create"))
      assert_response([[Result::UNKNOWN_ERROR, "should implement error case"]],
                      response,
                      :content_type => "application/json")

      assert_table_list([])
    end

    def test_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY,
                                  :value_type => "Int32"))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")

      assert_table_list([["users",
                          Flag::PERSISTENT | Table::PAT_KEY,
                          Type::VOID]])
    end

    def test_nonexistent_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY,
                                  :value_type => "nonexistent"))
      assert_response([[Result::UNKNOWN_ERROR, "should implement error case"]],
                      response,
                      :content_type => "application/json")

      assert_table_list([])
    end
  end

  class ArrayCreateTest < Test::Unit::TestCase
    include Utils

    def test_simple
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::NO_KEY))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")

      assert_table_list([["users",
                          Flag::PERSISTENT | Table::NO_KEY,
                          Type::VOID]])
    end

    def test_normalize_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::NO_KEY | Key::NORMALIZE))
      assert_response([[Result::UNKNOWN_ERROR,
                        "key normalization isn't available"]],
                      response,
                      :content_type => "application/json")

      assert_table_list([])
    end

    def test_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::NO_KEY,
                                  :key_type => "ShortText"))
      assert_response([[Result::UNKNOWN_ERROR, "key isn't supported"]],
                      response,
                      :content_type => "application/json")

      assert_table_list([])
    end

    def test_sis
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::NO_KEY | Key::SIS))
      assert_response([[Result::UNKNOWN_ERROR, "SIS key isn't available"]],
                      response,
                      :content_type => "application/json")

      assert_table_list([])
    end

    def test_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::NO_KEY,
                                  :value_type => "Int32"))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")

      assert_table_list([["users",
                          Flag::PERSISTENT | Table::NO_KEY,
                          Type::INT32]])
    end

    def test_view_value
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")
      users_table_id = object_registered

      response = get(command_path(:table_create,
                                  :name => "sites",
                                  :flags => Table::NO_KEY,
                                  :value_type => "users"))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")

      assert_table_list([["sites",
                          Flag::PERSISTENT | Table::NO_KEY,
                          users_table_id],
                         ["users",
                          Flag::PERSISTENT | Table::VIEW,
                          Type::VOID]])
    end

    def test_nonexistent_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::NO_KEY,
                                  :value_type => "nonexistent"))
      assert_response([[Result::UNKNOWN_ERROR, "value type doesn't exist"]],
                      response,
                      :content_type => "application/json")

      assert_table_list([])
    end
  end

  class ViewCreateTest < Test::Unit::TestCase
    include Utils

    def test_simple
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW))
      assert_response([[Result::SUCCESS]],
                      response,
                      :content_type => "application/json")

      assert_table_list([["users",
                          Flag::PERSISTENT | Table::VIEW,
                          Type::VOID]])
    end
    def test_normalize_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW | Key::NORMALIZE))
      assert_response([[Result::UNKNOWN_ERROR,
                        "key normalization isn't available"]],
                      response,
                      :content_type => "application/json")

      assert_table_list([])
    end

    def test_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW,
                                  :key_type => "ShortText"))
      assert_response([[Result::UNKNOWN_ERROR, "key isn't supported"]],
                      response,
                      :content_type => "application/json")

      assert_table_list([])
    end

    def test_sis
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW | Key::SIS))
      assert_response([[Result::UNKNOWN_ERROR, "SIS key isn't available"]],
                      response,
                      :content_type => "application/json")

      assert_table_list([])
    end

    def test_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW,
                                  :value_type => "Int32"))
      assert_response([[Result::UNKNOWN_ERROR, "value isn't available"]],
                      response,
                      :content_type => "application/json")

      assert_table_list([])
    end
  end
end
