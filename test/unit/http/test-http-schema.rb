# -*- coding: utf-8 -*-
#
# Copyright (C) 2009-2011  Kouhei Sutou <kou@clear-code.com>
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

module HTTPSchemaTests
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
      @bookmarks_table_id = table_create("bookmarks",
                                         :flags => Table::PAT_KEY,
                                         :key_type => "ShortText",
                                         :value_type => "Object",
                                         :default_tokenizer => "")
    end

    def create_bookmark_title_column
      @bookmarks_title_column_id = column_create("bookmarks",
                                                 "title",
                                                 Column::SCALAR,
                                                 "ShortText")
    end

    def assert_table_list(expected)
      response = get(command_path(:table_list, :output_type => output_type))
      expected = expected.collect do |values|
        id, name, flags, domain, range = values
        [id, name, nil, flags, domain, range]
      end
      assert_response_body([[["id", "UInt32"],
                             ["name", "ShortText"],
                             ["path", "ShortText"],
                             ["flags", "ShortText"],
                             ["domain", "ShortText"],
                             ["range", "ShortText"]],
                            *expected],
                           response,
                           :content_type => content_type) do |actual|
        status, result = actual
        header, *values = result
        values = values.collect do |value|
          id, name, path, flags, domain, range = value
          [id, name, nil, flags, domain, range]
        end
        [status, [header, *values]]
      end
    end

    def assert_column_list(expected, options)
      options = options.merge(:output_type => output_type)
      response = get(command_path(:column_list, options))
      expected = expected.collect do |values|
        id, name, type, flags, domain, range, source = values
        [id, name, nil, type, flags, domain, range, source]
      end
      assert_response_body([[["id", "UInt32"],
                             ["name", "ShortText"],
                             ["path", "ShortText"],
                             ["type", "ShortText"],
                             ["flags", "ShortText"],
                             ["domain", "ShortText"],
                             ["range", "ShortText"],
                             ["source", "ShortText"]],
                            *expected],
                           response,
                           :content_type => content_type) do |actual|
        status, result = actual
        header, *values = result
        values = values.collect do |value|
          id, name, path, type, flags, domain, range, source = value
          [id, name, nil, type, flags, domain, range, source]
        end
        [status, [header, *values]]
      end
    end
  end

  include Utils

  def test_table_list_with_invalid_output_type
    omit('now invalid output types are interpreted to json')
    response = get(command_path(:table_list,
                                :output_type => "unknown"))
    assert_response([[Result::UNKNOWN_ERROR, "should be implemented"]],
                    response,
                    :content_type => content_type)
  end

  def test_column_list_empty
    create_bookmarks_table
    assert_column_list([[@bookmarks_table_id,
                         "_key",
                         "",
                         "COLUMN_SCALAR",
                         "bookmarks",
                         "ShortText",
                         []]],
                       :table => "bookmarks")
  end

  def test_column_list_exist
    create_bookmarks_table
    create_bookmark_title_column
    assert_column_list([[@bookmarks_table_id,
                         "_key",
                         "",
                         "COLUMN_SCALAR",
                         "bookmarks",
                         "ShortText",
                          []],
                        [@bookmarks_title_column_id,
                         "title",
                         "var",
                         "COLUMN_SCALAR|PERSISTENT",
                         "bookmarks",
                         "ShortText",
                         []]],
                       :table => "bookmarks")
  end

  def test_column_list_nonexistent
    response = get(command_path(:column_list,
                                :table => "nonexistent",
                                :output_type => output_type))
    assert_error_response(Result::INVALID_ARGUMENT,
                          "table 'nonexistent' does not exist.",
                          response,
                          :content_type => content_type)
  end

  def test_column_list_without_table
    response = get(command_path(:column_list, :output_type => output_type))
    assert_error_response(Result::INVALID_ARGUMENT, "table '' does not exist.",
                          response,
                          :content_type => content_type)
  end

  def test_column_list_with_invalid_output_type
    omit('now invalid output types are interpreted to json')
    create_bookmarks_table
    response = get(command_path(:column_list,
                                :table => "bookmarks",
                                :output_type => "unknown"))
    assert_error_response(Result::UNKNOWN_ERROR, "should be implemented",
                          response,
                          :content_type => content_type)
  end

  def test_column_list_with_invalid_output_type_without_table
    omit('now invalid output types are interpreted to json')
    response = get(command_path(:column_list,
                                :output_type => "unknown"))
    assert_error_response(Result::UNKNOWN_ERROR, "should be implemented",
                          response,
                          :content_type => content_type)
  end

  def test_table_create_without_name
    response = get(command_path(:table_create, :output_type => output_type))
    assert_error_response(Result::INVALID_ARGUMENT,
                          "[table][create] " +
                            "should not create anonymous table",
                          response,
                          :content_type => content_type)
  end

  def test_table_create_with_dot_name
    response = get(command_path(:table_create,
                                :name => "mori.daijiro",
                                :output_type => output_type))
    assert_error_response(Result::INVALID_ARGUMENT,
                          "[table][create] " +
                          "name can't start with '_' " +
                          "and contains only 0-9, A-Z, a-z, #, @, - or _: " +
                          "<mori.daijiro>",
                          response,
                          :content_type => content_type)
  end

  def test_table_create_with_under_score_started_name
    response = get(command_path(:table_create,
                                :name => "_mori",
                                :output_type => output_type))
    assert_error_response(Result::INVALID_ARGUMENT,
                          "[table][create] " +
                          "name can't start with '_' " +
                          "and contains only 0-9, A-Z, a-z, #, @, - or _: " +
                          "<_mori>",
                          response,
                          :content_type => content_type)
  end

  def test_table_create_with_under_score_name
    response = get(command_path(:table_create,
                                :name => "mori_daijiro",
                                :output_type => output_type))
    assert_success_response(response, :content_type => content_type)
  end

  def test_table_create_with_colon_name
    response = get(command_path(:table_create,
                                :name => "daijiro:mori",
                                :output_type => output_type
                                ))
    assert_error_response(Result::INVALID_ARGUMENT,
                          "[table][create] " +
                          "name can't start with '_' " +
                          "and contains only 0-9, A-Z, a-z, #, @, - or _: " +
                          "<daijiro:mori>",
                          response,
                          :content_type => content_type)
  end

  def test_table_create_with_duplicated_name
    response = get(command_path(:table_create,
                                :name => "daijiro",
                                :output_type => output_type))
    assert_success_response(response, :content_type => content_type)
    response = get(command_path(:table_create,
                                :name => "daijiro",
                                :output_type => output_type))
    assert_error_response(Result::INVALID_ARGUMENT,
                          "already used name was assigned: <daijiro>",
                          response,
                          :content_type => content_type)
  end

  def test_full_text_search
    create_bookmarks_table
    create_bookmark_title_column

    table_create("terms",
                 :flags => Table::PAT_KEY | Key::NORMALIZE,
                 :key_type => "ShortText",
                 :default_tokenizer => "TokenBigram")
    column_create("terms", "bookmarks_title",
                  Column::INDEX | Flag::WITH_POSITION,
                  "bookmarks",
                  :source => "title")

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

  module HashCreateTests
    include Utils

    def test_simple
      table_id = table_create("users")
      assert_table_list([[table_id,
                          "users",
                          "TABLE_HASH_KEY|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_normalize_key
      table_id = table_create("users", :flags => Key::NORMALIZE)
      assert_table_list([[table_id,
                          "users",
                          "TABLE_HASH_KEY|KEY_NORMALIZE|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_normalized_string_key
      table_id = table_create("users",
                              :flags => Key::NORMALIZE,
                              :key_type => "ShortText")
      assert_table_list([[table_id,
                          "users",
                          "TABLE_HASH_KEY|KEY_NORMALIZE|PERSISTENT",
                          "ShortText",
                          "null"]])
    end

    def test_view_key
      users_table_id = table_create("users", :flags => Table::VIEW)
      sites_table_id = table_create("sites", :key_type => "users")
      assert_table_list([[sites_table_id,
                          "sites",
                          "TABLE_HASH_KEY|PERSISTENT",
                          "users",
                          "null"],
                         [users_table_id,
                          "users",
                          "TABLE_VIEW|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_big_size_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :key_type => "Text",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "key size too big: " +
                              "<users> <Text>(65536) (max:4096)",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_sis
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Key::SIS,
                                  :key_type => "ShortText",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "key with SIS isn't available " +
                              "for hash table: <users>",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_nonexistent_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :key_type => "nonexistent",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "key type doesn't exist: " +
                              "<users> (nonexistent)",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_invalid_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :key_type => "table_create",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "key type must be type or table: " +
                              "<users> (table_create)",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_value_type
      table_id = table_create("users", :value_type => "Int32")
      assert_table_list([[table_id,
                          "users",
                          "TABLE_HASH_KEY|PERSISTENT",
                          "null",
                          "Int32"]])
    end

    def test_nonexistent_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :value_type => "nonexistent",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "value type doesn't exist: " +
                              "<users> (nonexistent)",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_invalid_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :value_type => "table_create",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "value type must be type or table: " +
                              "<users> (table_create)",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_variable_size_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :value_type => "ShortText",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "value type must be fixed size: " +
                              "<users> (ShortText)",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end
  end

  module PatriciaTrieCreateTests
    include Utils

    def test_simple
      table_id = table_create("users", :flags => Table::PAT_KEY)
      assert_table_list([[table_id,
                          "users",
                          "TABLE_PAT_KEY|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_normalize_key
      table_id = table_create("users",
                              :flags => Table::PAT_KEY | Key::NORMALIZE)
      assert_table_list([[table_id,
                          "users",
                          "TABLE_PAT_KEY|KEY_NORMALIZE|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_normalized_string_key
      table_id = table_create("users",
                              :flags => Table::PAT_KEY | Key::NORMALIZE,
                              :key_type => "ShortText")
      assert_table_list([[table_id,
                          "users",
                          "TABLE_PAT_KEY|KEY_NORMALIZE|PERSISTENT",
                          "ShortText",
                          "null"]])
    end

    def test_view_key
      users_table_id = table_create("users", :flags => Table::VIEW)
      sites_table_id = table_create("sites",
                                    :flags => Table::PAT_KEY,
                                    :key_type => "users")
      assert_table_list([[sites_table_id,
                          "sites",
                          "TABLE_PAT_KEY|PERSISTENT",
                          "users",
                          "null"],
                         [users_table_id,
                          "users",
                          "TABLE_VIEW|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_big_size_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY,
                                  :key_type => "Text",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "key size too big: " +
                              "<users> <Text>(65536) (max:4096)",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_sis
      table_id = table_create("users",
                              :flags => Table::PAT_KEY | Key::SIS,
                              :key_type => "ShortText")
      assert_table_list([[table_id,
                          "users",
                          "TABLE_PAT_KEY|KEY_WITH_SIS|PERSISTENT",
                          "ShortText",
                          "null"]])
    end

    def test_nonexistent_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY,
                                  :key_type => "nonexistent",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "key type doesn't exist: " +
                              "<users> (nonexistent)",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_invalid_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY,
                                  :key_type => "table_create",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "key type must be type or table: " +
                              "<users> (table_create)",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_value_type
      table_id = table_create("users",
                              :flags => Table::PAT_KEY,
                              :value_type => "Int32")
      assert_table_list([[table_id,
                          "users",
                          "TABLE_PAT_KEY|PERSISTENT",
                          "null",
                          "Int32"]])
    end

    def test_nonexistent_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY,
                                  :value_type => "nonexistent",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "value type doesn't exist: " +
                              "<users> (nonexistent)",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_invalid_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY,
                                  :value_type => "table_create",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "value type must be type or table: " +
                              "<users> (table_create)",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_variable_size_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::PAT_KEY,
                                  :value_type => "ShortText",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "value type must be fixed size: " +
                              "<users> (ShortText)",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end
  end

  module ArrayCreateTests
    include Utils

    def test_simple
      table_id = table_create("users", :flags => Table::NO_KEY)
      assert_table_list([[table_id,
                          "users",
                          "TABLE_NO_KEY|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_normalize_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::NO_KEY | Key::NORMALIZE,
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "key normalization isn't available " +
                              "for no key table: <users>",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::NO_KEY,
                                  :key_type => "ShortText",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "key isn't available for no key table: " +
                              "<users> (ShortText)",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_sis
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::NO_KEY | Key::SIS,
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "key with SIS isn't available " +
                              "for no key table: <users>",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_value_type
      table_id = table_create("users",
                              :flags => Table::NO_KEY,
                              :value_type => "Int32")
      assert_table_list([[table_id,
                          "users",
                          "TABLE_NO_KEY|PERSISTENT",
                          "Int32",
                          "Int32"]])
    end

    def test_view_value
      users_table_id = table_create("users", :flags => Table::VIEW)
      sites_table_id = table_create("sites",
                                    :flags => Table::NO_KEY,
                                    :value_type => "users")
      assert_table_list([[sites_table_id,
                          "sites",
                          "TABLE_NO_KEY|PERSISTENT",
                          "null",
                          "users"],
                         [users_table_id,
                          "users",
                          "TABLE_VIEW|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_nonexistent_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::NO_KEY,
                                  :value_type => "nonexistent",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "value type doesn't exist: " +
                              "<users> (nonexistent)",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end
  end

  module ViewCreateTests
    include Utils

    def test_simple
      table_id = table_create("users", :flags => Table::VIEW)
      assert_table_list([[table_id,
                          "users",
                          "TABLE_VIEW|PERSISTENT",
                          "null",
                          "null"]])
    end
    def test_normalize_key
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW | Key::NORMALIZE,
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "key normalization isn't available " +
                              "for view table: <users>",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_key_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW,
                                  :key_type => "ShortText",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "key isn't available for view table: " +
                              "<users> (ShortText)",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_sis
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW | Key::SIS,
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "key with SIS isn't available " +
                              "for view table: <users>",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_value_type
      response = get(command_path(:table_create,
                                  :name => "users",
                                  :flags => Table::VIEW,
                                  :value_type => "Int32",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "[table][create] " +
                              "value isn't available for view table: " +
                              "<users> (Int32)",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end
  end

  module SymbolFlagsTests
    include Utils

    def test_table_create_single_symbol
      table_id = table_create("books", :flags => "KEY_NORMALIZE")
      assert_table_list([[table_id,
                          "books",
                          "TABLE_HASH_KEY|KEY_NORMALIZE|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_table_table_create_view
      table_id = table_create("books", :flags => "TABLE_VIEW")
      assert_table_list([[table_id,
                          "books",
                          "TABLE_VIEW|PERSISTENT",
                          "null",
                          "null"]])
    end

    def test_table_create_combined_symbols
      table_id = table_create("books",
                              :flags => "TABLE_HASH_KEY|KEY_NORMALIZE",
                              :key_type => "ShortText")
      assert_table_list([[table_id,
                          "books",
                          "TABLE_HASH_KEY|KEY_NORMALIZE|PERSISTENT",
                          "ShortText",
                          "null"]])
    end

    def test_table_create_combined_symbols_with_whitespaces
      table_id = table_create("books",
                              :flags => " TABLE_HASH_KEY | KEY_NORMALIZE ",
                              :key_type => "ShortText")
      assert_table_list([[table_id,
                          "books",
                          "TABLE_HASH_KEY|KEY_NORMALIZE|PERSISTENT",
                          "ShortText",
                          "null"]])
    end

    def test_table_create_invalid_symbol
      response = get(command_path(:table_create,
                                  :name => "books",
                                  :flags => "INVALID_SYMBOL",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "invalid flags option: INVALID_SYMBOL",
                            response,
                            :content_type => content_type)

      assert_table_list([])
    end

    def test_column_create_single_symbol
      create_books_table

      books_name_column_id = column_create("books",
                                           "name",
                                           "COLUMN_VECTOR",
                                           "ShortText")
      assert_column_list([[books_name_column_id,
                           "name",
                           "var",
                           "COLUMN_VECTOR|PERSISTENT",
                           "books",
                           "ShortText",
                           []]],
                         :table => "books")
    end

    def test_column_create_combined_symbols
      create_books_table

      books_name_column_id = column_create("books",
                                           "name",
                                           "COLUMN_INDEX|WITH_WEIGHT",
                                           "ShortText")
      assert_column_list([[books_name_column_id,
                           "name",
                           "index",
                           "COLUMN_INDEX|WITH_WEIGHT|PERSISTENT",
                           "books",
                           "ShortText",
                           []]],
                         :table => "books")
    end

    def test_column_create_combined_symbols_with_whitespaces
      create_books_table

      books_name_column_id = column_create("books",
                                           "name",
                                           " COLUMN_INDEX | WITH_WEIGHT ",
                                           "ShortText")
      assert_column_list([[books_name_column_id,
                           "name",
                           "index",
                           "COLUMN_INDEX|WITH_WEIGHT|PERSISTENT",
                           "books",
                           "ShortText",
                           []]],
                         :table => "books")
    end

    def test_column_create_invalid_symbol
      create_books_table

      response = get(command_path(:column_create,
                                  :table => "books",
                                  :name => "name",
                                  :flags => "INVALID_SYMBOL",
                                  :type => "ShortText",
                                  :output_type => output_type))
      assert_error_response(Result::INVALID_ARGUMENT,
                            "invalid flags option: INVALID_SYMBOL",
                            response,
                            :content_type => content_type)
      assert_column_list([], :table => "books")
    end

    private
    def create_books_table
      @books_table_id = table_create("books")

      assert_table_list([[@books_table_id,
                          "books",
                          "TABLE_HASH_KEY|PERSISTENT",
                          "null",
                          "null"]])
    end
  end
end

class JSONHTTPSchemaTests < Test::Unit::TestCase
  include HTTPSchemaTests
  include Format::JSON

  class JSONHashCreateTest < Test::Unit::TestCase
    include HTTPSchemaTests::HashCreateTests
    include Format::JSON
  end

  class JSONPatriciaTrieCreateTest < Test::Unit::TestCase
    include HTTPSchemaTests::PatriciaTrieCreateTests
    include Format::JSON
  end

  class JSONArrayCreateTest < Test::Unit::TestCase
    include HTTPSchemaTests::ArrayCreateTests
    include Format::JSON
  end

  class JSONViewCreateTest < Test::Unit::TestCase
    include HTTPSchemaTests::ViewCreateTests
    include Format::JSON
  end

  class JSONSymbolFlagsTest < Test::Unit::TestCase
    include HTTPSchemaTests::SymbolFlagsTests
    include Format::JSON
  end
end

class MessagePackHTTPSchemaTests < Test::Unit::TestCase
  include HTTPSchemaTests
  include Format::MessagePack

  class MessagePackHashCreateTest < Test::Unit::TestCase
    include HTTPSchemaTests::HashCreateTests
    include Format::MessagePack
  end

  class MessagePackPatriciaTrieCreateTest < Test::Unit::TestCase
    include HTTPSchemaTests::PatriciaTrieCreateTests
    include Format::MessagePack
  end

  class MessagePackArrayCreateTest < Test::Unit::TestCase
    include HTTPSchemaTests::ArrayCreateTests
    include Format::MessagePack
  end

  class MessagePackViewCreateTest < Test::Unit::TestCase
    include HTTPSchemaTests::ViewCreateTests
    include Format::MessagePack
  end

  class MessagePackSymbolFlagsTest < Test::Unit::TestCase
    include HTTPSchemaTests::SymbolFlagsTests
    include Format::MessagePack
  end
end
