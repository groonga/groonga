# -*- coding: utf-8 -*-
#
# Copyright (C) 2009-2011  Kouhei Sutou <kou@clear-code.com>
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

class StaticHTMLTest < Test::Unit::TestCase
  include GroongaHTTPTestUtils

  def setup
    setup_server
  end

  def teardown
    teardown_server
  end

  def test_normal
    response = get("/index.html")
    assert_response(File.read(File.join(document_root, "index.html")),
                    response,
                    :content_type => "text/html")
  end

  def test_with_fragment
    response = get("/index.html#anchor")
    assert_response(File.read(File.join(document_root, "index.html")),
                    response,
                    :content_type => "text/html")
  end

  def test_with_query
    response = get("/index.html?key=value")
    assert_response(File.read(File.join(document_root, "index.html")),
                    response,
                    :content_type => "text/html")
  end

  def test_with_query_and_fragment
    response = get("/index.html?key=value#anchor")
    assert_response(File.read(File.join(document_root, "index.html")),
                    response,
                    :content_type => "text/html")
  end

  def test_outside
    response = get("/../index.html")
    assert_response(File.read(File.join(document_root, "index.html")),
                    response,
                    :content_type => "text/html")
  end
end
