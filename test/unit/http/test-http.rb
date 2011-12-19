# -*- coding: utf-8 -*-
#
# Copyright (C) 2009-2011  Kouhei Sutou <kou@clear-code.com>
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

module HTTPTests
  include GroongaHTTPTestUtils

  class << self
    def included(base)
      base.setup :setup_server
      base.teardown :teardown_server
    end
  end

  def test_status
    response = get(command_path(:status, :output_type => output_type))
    assert_equal(content_type, response.content_type)
    assert_equal(["alloc_count", "cache_hit_rate", "command_version",
                  "default_command_version", "max_command_version", "n_queries",
                  "starttime", "uptime", "version"],
                 parse(response.body)[1].keys.sort)
  end

  def test_status_command_version
    response = get(command_path(:status,
                                :output_type => output_type,
                                :command_version => 1))
    assert_equal(content_type, response.content_type)
    assert_equal(1, parse(response.body)[1]["command_version"])
  end

  def test_quit
    response = get(command_path(:quit, :output_type => output_type))
    assert_success_response(response,
                            :content_type => content_type)

    assert_nothing_raised do
      get(command_path(:quit, :output_type => output_type))
    end
  end

  def test_shutdown
    response = get(command_path(:shutdown, :output_type => output_type))
    assert_success_response(response,
                            :content_type => content_type)
    @groonga_pid = nil

    assert_raise(Errno::ECONNREFUSED, Errno::ECONNRESET, EOFError) do
      get(command_path(:shutdown, :output_type => output_type))
    end
  end

  def test_nonexistent
    response = get(command_path(:nonexistent, :output_type => output_type))
    assert_equal(content_type, response.content_type)
    error_response = parse(response.body)
    error_response[0][1] = 0.0
    error_response[0][2] = 0.0
    if error_response[0][4]
      backtrace = error_response[0][4][0]
      backtrace[0] = "function" if backtrace[0].is_a?(String)
      backtrace[1] = "file.c" if backtrace[1].is_a?(String)
      backtrace[2] = 29 if backtrace[2].is_a?(Integer)
    end
    path = "#{document_root}/d/nonexistent"
    path << ".#{output_type}" if output_type
    assert_equal([[Result::NO_SUCH_FILE_OR_DIRECTORY,
                   0.0,
                   0.0,
                   "no such file: <#{path}>",
                   [["function", "file.c", 29]]]],
                 error_response)
  end
end

class JSONHTTPTest < Test::Unit::TestCase
  include HTTPTests
  include Format::JSON

  def test_jsonp
    response = get(command_path(:status,
                                :output_type => output_type,
                                :callback => "func"))
    assert_equal(content_type, response.content_type)
    assert_match(/\Afunc\(.+\);\z/, response.body)
  end
end

class MessagePackHTTPTest < Test::Unit::TestCase
  include HTTPTests
  include Format::MessagePack
end
