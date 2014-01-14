# -*- coding: utf-8 -*-
#
# Copyright (C) 2009-2014  Kouhei Sutou <kou@clear-code.com>
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

class InvalidHTTPTest < Test::Unit::TestCase
  include GroongaHTTPTestUtils

  def setup
    setup_server
  end

  def teardown
    teardown_server
  end

  def test_root
    response = get("/")
    assert_equal("200", response.code)
    path = File.join(document_root, 'index.html')
    assert_equal(utf8(File.read(path)), utf8(response.body))
  end

  def test_outside_html_outside_existent_inner_nonexistent
    relative_path = "../../Makefile.am"
    assert_true(File.exist?(File.join(document_root, relative_path)))
    assert_false(File.exist?(File.join(document_root,
                                       File.basename(relative_path))))

    response = get("/#{relative_path}")
    assert_equal("404", response.code)
  end

  def test_outside_html_with_invalid_utf8
    relative_path = "../../Makefile.am"
    assert_true(File.exist?(File.join(document_root, relative_path)))
    assert_false(File.exist?(File.join(document_root,
                                       File.basename(relative_path))))
    invalid_relative_path = relative_path.gsub(/\//, "\xC0\x2F")

    response = get("/#{invalid_relative_path}")
    assert_equal("404", response.code)
  end

  def test_symbolic_link
    relative_path = "../../Makefile.am"
    relative_symbolic_link_path = "link"
    path = File.join(document_root, relative_symbolic_link_path)
    symbolic_link_path = File.join(document_root, relative_symbolic_link_path)
    assert_false(File.exist?(symbolic_link_path))

    begin
      FileUtils.ln_s(relative_path, symbolic_link_path)
      assert_true(File.exist?(symbolic_link_path))
      assert_true(File.exist?(path))
      assert_equal(File.read(path), File.read(symbolic_link_path))

      response = get("/#{relative_symbolic_link_path}")
      assert_equal("404", response.code)
    ensure
      FileUtils.rm_f(symbolic_link_path)
    end
  end

  def test_not_start_with_slash
    response = get(".")
    assert_equal("400", response.code)
  end

  def test_long_path
    response = get("/0123456789" * 10000)
    assert_equal("400", response.code)
    assert_response([[Result::INVALID_ARGUMENT,
                      0.0,
                      0.0,
                      "too long path name: <PATH...> LENGTH(MAX_LENGTH)",
                      nil]],
                    response, :content_type => "application/json") do |_response|
      message = _response[0][3]
      message.gsub!(/<(.+?)\.\.\.>/, '<PATH...>')
      message.gsub!(/(\d+)\((\d+)\)/, 'LENGTH(MAX_LENGTH)')
      _response
    end
  end

  def test_long_query
    options = {}
    100.times do |i|
      options["key#{i}"] = "value#{i}"
    end
    response = get(command_path("status", options))
    assert_equal("200", response.code)
    # TODO: check body
  end

  def test_short_method
    omit('now groonga server cannot handle short method.')
    socket = TCPSocket.new(@address, @port)
    socket.print("G")
    socket.flush
    Timeout.timeout(1) do
      response = get(command_path("table_list"))
      assert_equal("200", response.code)
    end
  end
end
