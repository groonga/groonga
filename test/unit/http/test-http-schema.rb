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

  def test_table_list_created
    response = get(command_path(:table_create,
                                :name => "users",
                                :flags => Table::PAT_KEY,
                                :key_type => "Int8",
                                :value_type => "Object",
                                :default_tokenizer => ""))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "text/javascript")

    response = get(command_path(:table_list))
    assert_response([
                     ["id", "name", "path", "flags", "domain"],
                     [nil,
                      "users",
                      nil,
                      Flag::PERSISTENT | Table::PAT_KEY | Key::INT,
                      Type::INT8],
                    ],
                    response,
                    :content_type => "text/javascript") do |actual|
      actual[0, 1] + actual[1..-1].collect do |values|
        id, name, path, flags, domain = values
        [nil, name, nil, flags, domain]
      end
    end
  end
end
