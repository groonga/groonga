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

module HTTPCRUDTest
  class HTTPAddTest < Test::Unit::TestCase
    include GroongaHTTPTestUtils

    def setup
      setup_server
    end

    def teardown
      teardown_server
    end

    def test_normal
      populate_users

      response = get(command_path(:get,
                                  :table => "users",
                                  :key => "mori",
                                  :output_columns => "_key real_name"))
      assert_response([[Result::UNKNOWN_ERROR, "not found"]],
                      response,
                      :content_type => "application/json")

      response = get(command_path(:add,
                                  :table => "users",
                                  :key => "mori",
                                  :values => json({:real_name => "daijiro"}),
                                  :output_columns => "_key real_name"))
      assert_response([[Result::SUCCESS],
                       ["mori", "mori daijiro"]], response,
                      :content_type => "application/json")

      response = get(command_path(:get,
                                  :table => "users",
                                  :key => "mori",
                                  :output_columns => "_key real_name"))
      assert_response([[Result::SUCCESS],
                       ["mori", "daijiro"]],
                      response,
                      :content_type => "application/json")
    end

    def test_nonexistent_table
      response = get(command_path(:add,
                                  :table => "nonexistent",
                                  :key => "mori"))
      assert_response([[Result::UNKNOWN_ERROR, "table doesn't exist"]],
                      response,
                      :content_type => "application/json")
    end

    def test_no_key_for_no_key_table
      create_table("users", :flags => Table::NO_KEY)

      response = get(command_path(:add, :table => "users"))
      assert_response([[Result::SUCCESS], 1],
                      response,
                      :content_type => "application/json")
    end

    def test_no_key_for_key_table
      create_table("users",
                   :flags => Table::PAT_KEY,
                   :key_type => "ShortText")

      response = get(command_path(:add, :table => "users"))
      assert_response([[Result::UNKNOWN_ERROR, "key isn't required."]],
                      response,
                      :content_type => "application/json")
    end

    def test_values_object
      create_books_table

      response = get(command_path(:add,
                                  :table => "books",
                                  :key => "ruby",
                                  :values => json({"title" => "Ruby book",
                                                   "price" => 1000})))
      assert_response([[Result::SUCCESS],
                       1, "ruby", nil],
                      response,
                      :content_type => "application/json")
    end

    def test_values_array
      create_books_table

      response = get(command_path(:add,
                                  :table => "books",
                                  :key => "ruby",
                                  :columns => json(["title", "price"]),
                                  :values => json(["Ruby book", 1000])))
      assert_response([[Result::SUCCESS],
                       1, "ruby", nil],
                      response,
                      :content_type => "application/json")
    end

    def test_values_array_without_columns
      create_books_table

      response = get(command_path(:add,
                                  :table => "books",
                                  :key => "ruby",
                                  :values => json(["Ruby book", 1000])))
      assert_response([[Result::UNKNOWN_ERROR, "columns isn't specified"]],
                      response,
                      :content_type => "application/json")
    end

    def test_values_nonexistent_columns
      create_books_table

      response = get(command_path(:add,
                                  :table => "books",
                                  :key => "ruby",
                                  :values => json({"nonexistent" => "value"})))
      assert_response([[Result::UNKNOWN_ERROR, "unknown column"]],
                      response,
                      :content_type => "application/json")
    end

    def test_output_columns
      create_books_table

      response = get(command_path(:add,
                                  :table => "books",
                                  :key => "ruby",
                                  :columns => ["title"],
                                  :values => json(["Ruby book"]),
                                  :output_columns => "_key price title"))
      assert_response([[Result::SUCCESS],
                       "ruby", nil, "Ruby book"],
                      response,
                      :content_type => "application/json")
    end

    private
    def create_books_table
      table_create("books",
                   :flags => Table::PAT_KEY,
                   :key_type => "ShortText")
      column_create("books", "title", Column::SCALAR, "ShortText")
      column_create("books", "price", Column::SCALAR, "Int32")
    end
  end

  class HTTPSetTest < Test::Unit::TestCase
    include GroongaHTTPTestUtils

    def setup
      setup_server
    end

    def teardown
      teardown_server
    end

    def test_values
      populate_users

      response = get(command_path(:get,
                                  :table => "users",
                                  :key => "ryoqun",
                                  :output_columns => "_key real_name"))
      assert_response([[Result::SUCCESS],
                       ["ryoqun", "Ryo Onodera"]],
                      response,
                      :content_type => "application/json")

      response = get(command_path(:set,
                                  :table => "users",
                                  :key => "ryoqun",
                                  :values => json({:real_name => "daijiro"})))
      assert_response([[Result::SUCCESS]], response,
                      :content_type => "application/json")

      response = get(command_path(:get,
                                  :table => "users",
                                  :key => "ryoqun",
                                  :output_columns => "_key real_name"))
      assert_response([[Result::SUCCESS],
                       ["ryoqun", "daijiro"]],
                      response,
                      :content_type => "application/json")
    end

    def test_columns
      populate_users

      response = get(command_path(:get,
                                  :table => "users",
                                  :key => "ryoqun",
                                  :output_columns => "_key real_name"))
      assert_response([[Result::SUCCESS],
                       ["ryoqun", "Ryo Onodera"]],
                      response,
                      :content_type => "application/json")

      response = get(command_path(:set,
                                  :table => "users",
                                  :key => "ryoqun",
                                  :columns => json([:real_name]),
                                  :values => json(["daijiro"])))
      assert_response([[Result::SUCCESS]], response,
                      :content_type => "application/json")

      response = get(command_path(:get,
                                  :table => "users",
                                  :key => "ryoqun",
                                  :output_columns => "_key real_name"))
      assert_response([[Result::SUCCESS],
                       ["ryoqun", "daijiro"]],
                      response,
                      :content_type => "application/json")
    end

    def test_nonexistent_table
      response = get(command_path(:set,
                                  :table => "nonexistent",
                                  :key => "mori",
                                  :values => json({:_value => "mori daijiro"})))
      assert_response([[Result::UNKNOWN_ERROR, "table doesn't exist"]],
                      response,
                      :content_type => "application/json")
    end

    def test_nonexistent_key
      create_table("users",
                   :flags => Table::PAT_KEY,
                   :key_type => "ShortText")
      column_create("users", "name", Column::SCALAR, "ShortText")

      response = get(command_path(:set,
                                  :table => "users",
                                  :key => "mori",
                                  :values => json({:name => "daijiro"})))
      assert_response([[Result::UNKNOWN_ERROR, "entry doesn't exist"]],
                      response,
                      :content_type => "application/json")
    end

    def test_nonexistent_id
      create_table("users", :flags => Table::NO_KEY)
      column_create("users", "name", Column::SCALAR, "ShortText")

      response = get(command_path(:set,
                                  :table => "users",
                                  :id => 1,
                                  :values => json({:name => "daijiro"})))
      assert_response([[Result::UNKNOWN_ERROR, "entry doesn't exist"]],
                      response,
                      :content_type => "application/json")
    end

    def test_no_key_for_key_table
      create_table("users",
                   :flags => Table::PAT_KEY,
                   :key_type => "ShortText")
      column_create("users", "name", Column::SCALAR, "ShortText")

      response = get(command_path(:set,
                                  :table => "users",
                                  :values => json({:name => "daijiro"})))
      assert_response([[Result::UNKNOWN_ERROR, "key nor ID isn't specified"]],
                      response,
                      :content_type => "application/json")
    end

    def test_no_id_for_no_key_table
      create_table("users", :flags => Table::NO_KEY)
      column_create("users", "name", Column::SCALAR, "ShortText")

      response = get(command_path(:set,
                                  :table => "users",
                                  :values => json({:name => "daijiro"})))
      assert_response([[Result::UNKNOWN_ERROR, "ID isn't specified"]],
                      response,
                      :content_type => "application/json")
    end

    def test_no_values_with_key
      create_table("users",
                   :flags => Table::PAT_KEY,
                   :key_type => "ShortText")
      column_create("users", "name", Column::SCALAR, "ShortText")

      load("users", [{:key => "mori", :name => "daijiro"}])
      response = get(command_path(:set,
                                  :table => "users",
                                  :key => "mori"))
      assert_response([[Result::UNKNOWN_ERROR, "values isn't specified"]],
                      response,
                      :content_type => "application/json")
    end

    def test_no_values_with_id
      create_table("users", :flags => Table::NO_KEY)
      column_create("users", "name", Column::SCALAR, "ShortText")

      load("users", [{:name => "daijiro"}])
      response = get(command_path(:set,
                                  :table => "users",
                                  :id => 1))
      assert_response([[Result::UNKNOWN_ERROR, "values isn't specified"]],
                      response,
                      :content_type => "application/json")
    end
  end

  class HTTPGetTest < Test::Unit::TestCase
    include GroongaHTTPTestUtils

    def setup
      setup_server
    end

    def teardown
      teardown_server
    end

    def test_nonexistent_table
      response = get(command_path(:get,
                                  :table => "nonexistent",
                                  :key => "mori"))
      assert_response([[Result::UNKNOWN_ERROR, "table doesn't exist"]],
                      response,
                      :content_type => "application/json")
    end

    def test_no_table
      response = get(command_path(:get))
      assert_response([[Result::UNKNOWN_ERROR, "table isn't specified"]],
                      response,
                      :content_type => "application/json")
    end

    def test_invalid_table
      response = get(command_path(:get, :table => "Int32"))
      assert_response([[Result::UNKNOWN_ERROR, "not a table"]],
                      response,
                      :content_type => "application/json")
    end

    def test_no_key
      create_users_table
      response = get(command_path(:get, :table => "users"))
      assert_response([[Result::UNKNOWN_ERROR, "ID nor key isn't specified"]],
                      response,
                      :content_type => "application/json")
    end

    def test_key_for_array
      table_create("users", :flags => Table::NO_KEY)

      response = get(command_path(:get,
                                  :table => "users",
                                  :key => "morita"))
      assert_response([[Result::UNKNOWN_ERROR, "should not specify key"]],
                      response,
                      :content_type => "application/json")
    end

    def test_id_and_key
      table_create("users",
                   :flags => Table::PAT_KEY,
                   :key_type => "ShortText")

      load("users",
           [{:_key => "morita"},
            {:_key => "gunyara-kun"}])
      response = get(command_path(:get,
                                  :table => "users",
                                  :key => "morita",
                                  :id => 2))
      assert_response([[Result::UNKNOWN_ERROR,
                        "should not specify both key and ID"]],
                      response,
                      :content_type => "application/json")
    end

    def test_id_for_array
      table_create("users", :flags => Table::NO_KEY)
      column_create("users", "name", Column::SCALAR, "ShortText")

      load("users",
           [{:name => "morita"},
            {:name => "gunyara-kun"}])
      response = get(command_path(:get,
                                  :table => "users",
                                  :id => 2,
                                  :output_columns => "name"))
      assert_response([[Result::SUCCESS], ["gunyara-kun"]],
                      response,
                      :content_type => "application/json")
    end

    def test_id_for_key_table
      table_create("users",
                   :flags => Table::PAT_KEY,
                   :key_type => "ShortText")

      load("users",
           [{:_key => "morita"},
            {:_key => "gunyara-kun"}])
      response = get(command_path(:get,
                                  :table => "users",
                                  :id => 2,
                                  :output_columns => "_key"))
      assert_response([[Result::SUCCESS], ["gunyara-kun"]],
                      response,
                      :content_type => "application/json")
    end

    def test_key_for_key_table
      table_create("users",
                   :flags => Table::PAT_KEY,
                   :key_type => "ShortText")

      load("users",
           [{:_key => "morita"},
            {:_key => "gunyara-kun"}])
      response = get(command_path(:get,
                                  :table => "users",
                                  :id => "morita",
                                  :output_columns => "_id _key"))
      assert_response([[Result::SUCCESS], [1, "morita"]],
                      response,
                      :content_type => "application/json")
    end
  end
end
