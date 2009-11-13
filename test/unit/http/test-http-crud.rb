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
  end

  class HTTPSetTest < Test::Unit::TestCase
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

    def test_nonexistent_table
      response = get(command_path(:set,
                                  :table => "nonexistent",
                                  :key => "mori",
                                  :values => json({:_value => "mori daijiro"})))
      assert_response([[Result::UNKNOWN_ERROR, "table doesn't exist"]],
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
  end
end
