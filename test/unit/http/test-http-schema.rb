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
                    :content_type => "text/javascript")
  end

  def test_table_list_exist
    create_bookmarks_table
    response = get(command_path(:table_list))
    assert_response([
                     ["id", "name", "path", "flags", "domain"],
                     [@bookmarks_table_id,
                      "bookmarks",
                      nil,
                      Flag::PERSISTENT | Table::PAT_KEY | Key::INT,
                      Type::INT8],
                    ],
                    response,
                    :content_type => "text/javascript") do |actual|
      actual[0, 1] + actual[1..-1].collect do |values|
        id, name, path, flags, domain = values
        [id, name, nil, flags, domain]
      end
    end
  end

  def test_column_list_empty
    create_bookmarks_table
    response = get(command_path(:column_list,
                                :table => "bookmarks"))
    assert_response([["id", "name", "path", "type", "flags", "domain"]],
                    response,
                    :content_type => "text/javascript")
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
                    :content_type => "text/javascript") do |actual|
      actual[0, 1] + actual[1..-1].collect do |values|
        id, name, path, type, flags, domain = values
        [id, name, nil, type, flags, domain]
      end
    end
  end

  private
  def create_bookmarks_table
    response = get(command_path(:table_create,
                                :name => "bookmarks",
                                :flags => Table::PAT_KEY,
                                :key_type => "Int8",
                                :value_type => "Object",
                                :default_tokenizer => ""))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "text/javascript")
    @bookmarks_table_id = object_registered
  end

  def create_bookmark_title_column
    response = get(command_path(:column_create,
                                :table => "bookmarks",
                                :name => "title",
                                :flags => Column::SCALAR,
                                :type => "ShortText"))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "text/javascript")
    @bookmarks_title_column_id = object_registered
  end
end
