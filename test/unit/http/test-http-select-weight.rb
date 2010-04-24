# -*- coding: utf-8 -*-
#
# Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
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

class HTTPSelectWeightTest < Test::Unit::TestCase
  include GroongaHTTPTestUtils

  def setup
    setup_server
  end

  def teardown
    teardown_server
  end

  def test_weight
    populate_users

    assert_select([["_key", "ShortText"],
                   ["_score", "Int32"]],
                  [["hayamiz", 10]],
                  :table => "users",
                  :output_columns => "_key _score",
                  :match_columns => "real_name * 10",
                  :query => "Yuto")
  end

  def test_weight_minus
    omit("Is this test right?")
    populate_users

    assert_select([["_key", "ShortText"],
                   ["_score", "Int32"]],
                  [["hayamiz", -10]],
                  :table => "users",
                  :output_columns => "_key _score",
                  :match_columns => "real_name * -10",
                  :query => "Yuto")
  end
end
