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
      assert_success_response(response, :content_type => "application/json")
      @bookmarks_table_id = object_registered
    end

    def create_bookmark_title_column
      response = get(command_path(:column_create,
                                  :table => "bookmarks",
                                  :name => "title",
                                  :flags => Column::SCALAR,
                                  :type => "ShortText"))
      assert_success_response(response, :content_type => "application/json")
      @bookmarks_title_column_id = object_registered
    end

    def assert_table_list(expected)
      response = get(command_path(:table_list))
      expected = expected.collect do |values|
        name, flags, domain, range = values
        [nil, name, nil, flags, domain, range]
      end
      assert_response([
                       [["id", "UInt32"],
                        ["name", "ShortText"],
                        ["path", "ShortText"],
                        ["flags", "ShortText"],
                        ["domain", "ShortText"],
                        ["range", "ShortText"]],
                       *expected
                      ],
                      response,
                      :content_type => "application/json") do |actual|
        actual[0, 1] + actual[1..-1].collect do |values|
          id, name, path, flags, domain, range = values
          [nil, name, nil, flags, domain, range]
        end
      end
    end
  end

  include Utils

  def test_table_list_empty
    response = get(command_path(:table_list))
    assert_table_list([])
  end

  def test_table_list_exist
    create_bookmarks_table

    response = get(command_path(:table_list))
    normalized_path = "/path/to/table"
    assert_response([
                     [["id", "UInt32"],
                      ["name", "ShortText"],
                      ["path", "ShortText"],
                      ["flags", "ShortText"],
                      ["domain", "ShortText"],
                      ["range", "ShortText"]],
                     [@bookmarks_table_id,
                      "bookmarks",
                      normalized_path,
                      "TABLE_PAT_KEY|PERSISTENT",
                      "ShortText",
                      "Object"],
                    ],
                    response,
                    :content_type => "application/json") do |actual|
      actual[0, 1] + actual[1..-1].collect do |values|
        id, name, path, flags, domain, range = values
        path = normalized_path if path
        [id, name, path, flags, domain, range]
      end
    end
  end

  def test_table_list_with_invalid_output_type
    response = get(command_path(:table_list,
                                :output_type => "unknown"))
    assert_response([[Result::UNKNOWN_ERROR, "should be implemented"]],
                    response,
                    :content_type => "application/json")
  end

  def test_column_list_empty
    create_bookmarks_table
    response = get(command_path(:column_list,
                                :table => "bookmarks"))
    assert_response([[["id", "UInt32"],
                      ["name", "ShortText"],
                      ["path", "ShortText"],
                      ["type", "ShortText"],
                      ["flags", "ShortText"],
                      ["domain", "ShortText"],
                      ["range", "ShortText"],
                      ["source", "ShortText"]]],
                    response,
                    :content_type => "application/json")
  end

  def test_column_list_exist
    create_bookmarks_table
    create_bookmark_title_column
    response = get(command_path(:column_list,
                                :table => "bookmarks"))
    assert_response([
                     [["id", "UInt32"],
                      ["name", "ShortText"],
                      ["path", "ShortText"],
                      ["type", "ShortText"],
                      ["flags", "ShortText"],
                      ["domain", "ShortText"],
                      ["range", "ShortText"],
                      ["source", "ShortText"]],
                     [@bookmarks_title_column_id,
                      "title",
                      nil,
                      "var",
                      "COLUMN_SCALAR|COMPRESS_NONE|PERSISTENT",
                      "bookmarks",
                      "ShortText",
                      []]
                     ],
                    response,
                    :content_type => "application/json") do |actual|
      actual[0, 1] + actual[1..-1].collect do |values|
        id, name, path, type, flags, domain, range, source = values
        [id, name, nil, type, flags, domain, range, source]
      end
    end
  end

  def test_column_list_nonexistent
    response = get(command_path(:column_list,
                                :table => "nonexistent"))
    assert_error_response(Result::UNKNOWN_ERROR, "should be implemented",
                          response,
                          :content_type => "application/json")
  end

  def test_column_list_without_table
    response = get(command_path(:column_list))
    assert_error_response(Result::UNKNOWN_ERROR, "should be implemented",
                          response,
                          :content_type => "application/json")
  end

  def test_column_list_with_invalid_output_type
    create_bookmarks_table
    response = get(command_path(:column_list,
                                :table => "bookmarks",
                                :output_type => "unknown"))
    assert_error_response(Result::UNKNOWN_ERROR, "should be implemented",
                          response,
                          :content_type => "application/json")
  end

  def test_column_list_with_invalid_output_type_without_table
    response = get(command_path(:column_list,
                                :output_type => "unknown"))
    assert_error_response(Result::UNKNOWN_ERROR, "should be implemented",
                          response,
                          :content_type => "application/json")
  end

  def test_table_create_without_name
    response = get(command_path(:table_create))
    assert_error_response(Result::UNKNOWN_ERROR,
                          "should not create anonymous table",
                          response,
                          :content_type => "application/json")
  end

  def test_table_create_with_dot_name
    response = get(command_path(:table_create, :name => "mori.daijiro"))
    assert_error_response(Result::INVALID_ARGUMENT,
                          "name can't start with '_' and contains '.' or ':'",
                          response,
                          :content_type => "application/json")
  end

  def test_table_create_with_under_score_started_name
    response = get(command_path(:table_create, :name => "_mori"))
    assert_error_response(Result::INVALID_ARGUMENT,
                          "name can't start with '_' and contains '.' or ':'",
                          response,
                          :content_type => "application/json")
  end

  def test_table_create_with_under_score_name
    response = get(command_path(:table_create, :name => "mori_daijiro"))
    assert_success_response(response, :content_type => "application/json")
  end

  def test_table_create_with_colon_name
    response = get(command_path(:table_create, :name => "daijiro:mori"))
    assert_error_response(Result::INVALID_ARGUMENT,
                          "name can't start with '_' and contains '.' or ':'",
                          response,
                          :content_type => "application/json")
  end

  def test_table_create_with_duplicated_name
    response = get(command_path(:table_create, :name => "table_create"))
    assert_error_response(Result::INVALID_ARGUMENT,
                          "already used name was assigned",
                          response,
                          :content_type => "application/json")
  end

  def test_table_create_with_duplicated_name
    response = get(command_path(:table_create, :name => "table_create"))
    assert_error_response(Result::INVALID_ARGUMENT,
                          "already used name was assigned",
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
    assert_success_response(response, :content_type => "application/json")

    response = get(command_path(:column_create,
                                :table => "terms",
                                :name => "bookmarks-title",
                                :flags => Column::INDEX | Flag::WITH_POSITION,
                                :type => "bookmarks",
                                :source => "title"))
    assert_success_response(response, :content_type => "application/json")

    groonga_title = "groonga - an open-source fulltext search engine " +
                    "and column store."
    senna_title = "Senna: An Embeddable Fulltext Search Engine"
    load("bookmarks",
         [{"_key" => "groonga", "title" => groonga_title},
          {"_key" => "senna", "title" => senna_title}])

    assert_select([["_key", "ShortText"],
                   ["title", "ShortText"]],
                  [["groonga", groonga_title]],
                  :table => "bookmarks",
                  :output_columns => "_key title",
                  :query => "title:@column")
  end

  class HashCreateTest < Test::Unit::TestCase
    include Utils

    def test_simple
      response = get(command_path(:table_create, :name => "users"))
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["users",
                          "TABLE_HASH_KEY|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_normalize_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Key::NORMALIZE))
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["users",
                          "TABLE_HASH_KEY|KEY_NORMALIZE|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_normalized_string_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Key::NORMALIZE,
                                  :key_type => "ShortText"))
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["users",
                          "TABLE_HASH_KEY|KEY_NORMALIZE|PERSISTENT",
                          "ShortText",
                          "null"]])
    end

    def test_view_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW))
      assert_success_response(response, :content_type => "application/json")
      users_table_id = object_registered

      response = get(command_path(:table_create,
                                  :name => "sites",
                                  :key_type => "users"))
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["sites",
                          "TABLE_HASH_KEY|PERSISTENT",
                          "users",
                          "null"],
                         ["users",
                          "TABLE_VIEW|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_long_size_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :key_type => "Text"))
      assert_error_response(Result::UNKNOWN_ERROR,
                            "should implement error case",
                            response,
                            :content_type => "application/json")

      assert_table_list([])
    end

    def test_sis
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Key::SIS,
                                  :key_type => "ShortText"))
      assert_error_response(Result::UNKNOWN_ERROR,
                            "SIS is invalid flag for hash",
                            response,
                            :content_type => "application/json")

      assert_table_list([])
    end

    def test_nonexistent_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :key_type => "nonexistent"))
      assert_error_response(Result::UNKNOWN_ERROR,
                            "should implement error case",
                            response,
                            :content_type => "application/json")

      assert_table_list([])
    end

    def test_invalid_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :key_type => "table_create"))
      assert_error_response(Result::UNKNOWN_ERROR,
                            "should implement error case",
                            response,
                            :content_type => "application/json")

      assert_table_list([])
    end

    def test_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :value_type => "Int32"))
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["users",
                          "TABLE_HASH_KEY|PERSISTENT",
                          "null",
                          "Int32"]])
    end

    def test_nonexistent_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :value_type => "nonexistent"))
      assert_error_response(Result::UNKNOWN_ERROR,
                            "should implement error case",
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
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["users",
                          "TABLE_PAT_KEY|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_normalize_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY | Key::NORMALIZE))
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["users",
                          "TABLE_PAT_KEY|KEY_NORMALIZE|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_normalized_string_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY | Key::NORMALIZE,
                                  :key_type => "ShortText"))
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["users",
                          "TABLE_PAT_KEY|KEY_NORMALIZE|PERSISTENT",
                          "ShortText",
                          "null"]])
    end

    def test_view_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW))
      assert_success_response(response, :content_type => "application/json")
      users_table_id = object_registered

      response = get(command_path(:table_create,
                                  :name => "sites",
                                  :flags => Table::PAT_KEY,
                                  :key_type => "users"))
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["sites",
                          "TABLE_PAT_KEY|PERSISTENT",
                          "users",
                          "null"],
                         ["users",
                          "TABLE_VIEW|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_long_size_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY,
                                  :key_type => "Text"))
      assert_error_response(Result::UNKNOWN_ERROR,
                            "should implement error case",
                            response,
                            :content_type => "application/json")

      assert_table_list([])
    end

    def test_sis
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY | Key::SIS,
                                  :key_type => "ShortText"))
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["users",
                          "TABLE_PAT_KEY|KEY_WITH_SIS|PERSISTENT",
                          "ShortText",
                          "null"]])
    end

    def test_nonexistent_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY,
                                  :key_type => "nonexistent"))
      assert_error_response(Result::UNKNOWN_ERROR,
                            "should implement error case",
                            response,
                            :content_type => "application/json")

      assert_table_list([])
    end

    def test_invalid_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY,
                                  :key_type => "table_create"))
      assert_error_response(Result::UNKNOWN_ERROR,
                            "should implement error case",
                            response,
                            :content_type => "application/json")

      assert_table_list([])
    end

    def test_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY,
                                  :value_type => "Int32"))
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["users",
                          "TABLE_PAT_KEY|PERSISTENT",
                          "null",
                          "Int32"]])
    end

    def test_nonexistent_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY,
                                  :value_type => "nonexistent"))
      assert_error_response(Result::UNKNOWN_ERROR,
                            "should implement error case",
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
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["users",
                          "TABLE_NO_KEY|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_normalize_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::NO_KEY | Key::NORMALIZE))
      assert_error_response(Result::UNKNOWN_ERROR,
                            "key normalization isn't available",
                            response,
                            :content_type => "application/json")

      assert_table_list([])
    end

    def test_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::NO_KEY,
                                  :key_type => "ShortText"))
      assert_error_response(Result::UNKNOWN_ERROR,
                            "key isn't supported",
                            response,
                            :content_type => "application/json")

      assert_table_list([])
    end

    def test_sis
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::NO_KEY | Key::SIS))
      assert_error_response(Result::UNKNOWN_ERROR,
                            "SIS key isn't available",
                            response,
                            :content_type => "application/json")

      assert_table_list([])
    end

    def test_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::NO_KEY,
                                  :value_type => "Int32"))
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["users",
                          "TABLE_NO_KEY|PERSISTENT",
                          "Int32",
                          "Int32"]])
    end

    def test_view_value
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW))
      assert_success_response(response, :content_type => "application/json")
      users_table_id = object_registered

      response = get(command_path(:table_create,
                                  :name => "sites",
                                  :flags => Table::NO_KEY,
                                  :value_type => "users"))
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["sites",
                          "TABLE_NO_KEY|PERSISTENT",
                          "null",
                          "users"],
                         ["users",
                          "TABLE_VIEW|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_nonexistent_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::NO_KEY,
                                  :value_type => "nonexistent"))
      assert_error_response(Result::UNKNOWN_ERROR,
                            "value type doesn't exist",
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
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["users",
                          "TABLE_VIEW|PERSISTENT",
                          "null",
                          "null"]])
    end
    def test_normalize_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW | Key::NORMALIZE))
      assert_error_response(Result::UNKNOWN_ERROR,
                            "key normalization isn't available",
                            response,
                            :content_type => "application/json")

      assert_table_list([])
    end

    def test_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW,
                                  :key_type => "ShortText"))
      assert_error_response(Result::UNKNOWN_ERROR,
                            "key isn't supported",
                            response,
                            :content_type => "application/json")

      assert_table_list([])
    end

    def test_sis
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW | Key::SIS))
      assert_error_response(Result::UNKNOWN_ERROR,
                            "SIS key isn't available",
                            response,
                            :content_type => "application/json")

      assert_table_list([])
    end

    def test_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW,
                                  :value_type => "Int32"))
      assert_error_response(Result::UNKNOWN_ERROR,
                            "value isn't available",
                            response,
                            :content_type => "application/json")

      assert_table_list([])
    end
  end

  class SymbolFlagsTest < Test::Unit::TestCase
    include Utils

    def test_table_create_single_symbol
      response = get(command_path(:table_create,
                                  :name => "books",
                                  :flags => "KEY_NORMALIZE"))
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["books",
                          "TABLE_HASH_KEY|KEY_NORMALIZE|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_table_create_table_view
      response = get(command_path(:table_create,
                                  :name => "books",
                                  :flags => "TABLE_VIEW"))
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["books",
                          "TABLE_VIEW|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_table_create_combined_symbols
      response = get(command_path(:table_create,
                                  :name => "books",
                                  :flags => "TABLE_NO_KEY|KEY_NORMALIZE"))
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["books",
                          "TABLE_NO_KEY|KEY_NORMALIZE|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_table_create_combined_symbols_with_whitespaces
      response = get(command_path(:table_create,
                                  :name => "books",
                                  :flags => " TABLE_NO_KEY | KEY_NORMALIZE "))
      assert_success_response(response, :content_type => "application/json")

      assert_table_list([["books",
                          "TABLE_NO_KEY|KEY_NORMALIZE|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_table_create_invalid_symbol
      response = get(command_path(:table_create,
                                  :name => "books",
                                  :flags => "INVALID_SYMBOL"))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "invalid flags option: INVALID_SYMBOL",
                            response,
                            :content_type => "application/json")

      assert_table_list([])
    end

    def test_column_create_single_symbol
      create_books_table

      response = get(command_path(:column_create,
                                  :table => "books",
                                  :name => "name",
                                  :flags => "COLUMN_VECTOR",
                                  :type => "ShortText"))
      assert_success_response(response, :content_type => "application/json")
      books_name_column_id = object_registered

      assert_column_list([[books_name_column_id,
                           "name",
                           "var",
                           "COLUMN_VECTOR|COMPRESS_NONE|PERSISTENT",
                           "books",
                           "ShortText",
                           []]])
    end

    def test_column_create_combined_symbols
      create_books_table

      response = get(command_path(:column_create,
                                  :table => "books",
                                  :name => "name",
                                  :flags => "COLUMN_INDEX|WITH_WEIGHT",
                                  :type => "ShortText"))
      assert_success_response(response, :content_type => "application/json")
      books_name_column_id = object_registered

      assert_column_list([[books_name_column_id,
                           "name",
                           "index",
                           "WITH_WEIGHT|PERSISTENT",
                           "books",
                           []]])
    end

    def test_column_create_combined_symbols_with_whitespaces
      create_books_table

      response = get(command_path(:column_create,
                                  :table => "books",
                                  :name => "name",
                                  :flags => " COLUMN_INDEX | WITH_WEIGHT ",
                                  :type => "ShortText"))
      assert_success_response(response, :content_type => "application/json")
      books_name_column_id = object_registered

      assert_column_list([[books_name_column_id,
                           "name",
                           "index",
                           "COLUMN_INDEX|WITH_WEIGHT|PERSISTENT",
                           "books"]])
    end

    def test_column_create_invalid_symbol
      create_books_table

      response = get(command_path(:column_create,
                                  :table => "books",
                                  :name => "name",
                                  :flags => "INVALID_SYMBOL",
                                  :type => "ShortText"))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "invalid flags option: INVALID_SYMBOL",
                            response,
                            :content_type => "application/json")
      assert_column_list([])
    end

    private
    def assert_column_list(expected)
      response = get(command_path(:column_list, :table => "books"))
      expected = expected.collect do |values|
        id, name, type, flags, domain, range, source = values
        [id, name, nil, type, flags, domain, range, source]
      end
      assert_response([
                       [["id", "UInt32"],
                        ["name", "ShortText"],
                        ["path", "ShortText"],
                        ["type", "ShortText"],
                        ["flags", "ShortText"],
                        ["domain", "ShortText"],
                        ["range", "ShortText"],
                        ["source", "ShortText"]],
                       *expected
                      ],
                      response,
                      :content_type => "application/json") do |actual|
        actual[0, 1] + actual[1..-1].collect do |values|
          id, name, path, type, flags, domain, range, source = values
          [id, name, nil, type, flags, domain, range, source]
        end
      end
    end

    def create_books_table
      response = get(command_path(:table_create, :name => "books"))
      assert_success_response(response, :content_type => "application/json")
      @books_table_id = object_registered

      assert_table_list([["books",
                          "TABLE_HASH_KEY|PERSISTENT",
                          "null",
                          "null"]])
    end
  end
end
