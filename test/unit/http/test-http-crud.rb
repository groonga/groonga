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
  end
end
