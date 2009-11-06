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

class HTTPTest < Test::Unit::TestCase
  include GroongaTestUtils

  def setup
    setup_server
  end

  def teardown
    teardown_server
  end

  def test_root
    Net::HTTP.start(@address, @port) do |http|
      response = http.get("/")
      assert_equal("text/javascript", response.content_type)
      assert_equal("", response.body)
    end
  end

  def test_status
    Net::HTTP.start(@address, @port) do |http|
      response = http.get("/d/status")
      assert_equal("text/javascript", response.content_type)
      assert_equal(["alloc_count", "starttime", "uptime"],
                   JSON.parse(response.body).keys.sort)
    end
  end
end
