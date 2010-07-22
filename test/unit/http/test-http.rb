# -*- coding: utf-8 -*-
#
# Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>
# Copyright (C) 2009  Yuto HAYAMIZU <y.hayamizu@gmail.com>
# Copyright (C) 2009  Ryo Onodera <onodera@clear-code.com>
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
  include GroongaHTTPTestUtils

  def setup
    setup_server
  end

  def teardown
    teardown_server
  end

  def test_status
    response = get(command_path(:status))
    assert_equal("application/json", response.content_type)
    assert_equal(["alloc_count", "cache_hit_rate", "n_queries",
                  "starttime", "uptime", "version"],
                 JSON.parse(response.body)[1].keys.sort)
  end

  def test_quit
    response = get(command_path(:quit))
    assert_success_response(response,
                            :content_type => "application/json")

    assert_nothing_raised do
      get(command_path(:quit))
    end
  end

  def test_shutdown
    response = get(command_path(:shutdown))
    assert_success_response(response,
                            :content_type => "application/json")
    @groonga_pid = nil

    assert_raise(Errno::ECONNREFUSED, EOFError) do
      get(command_path(:shutdown))
    end
  end
end
