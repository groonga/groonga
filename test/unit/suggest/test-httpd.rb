# -*- coding: utf-8 -*-
#
# Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>
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

class SuggestHTTPDTest < Test::Unit::TestCase
  include GroongaHTTPTestUtils

  setup :setup_database_path
  setup :setup_dataset
  setup :setup_httpd

  teardown :teardown_httpd
  teardown :teardown_database_path

  def setup_dataset
    @dataset_name = "shops"
    system(groonga_suggest_create_dataset, @database_path, @dataset_name)
  end

  def setup_httpd
    @bind_address = "127.0.0.1"
    @port = 8080
    start_httpd
  end

  def teardown_httpd
    @groonga_suggest_httpd_pid ||= nil
    stop_server_process(@groonga_suggest_httpd_pid)
    @groonga_suggest_httpd_pid = nil
  end

  def test_complete
    options = {
      "q" => "wo",
      "s" => Time.now.to_i * 1000,
      "i" => "id",
      "t" => "complete",
      "s" => @dataset_name,
      "l" => @dataset_name,
    }
    encoded_options = encode_options(options)
    response = get("/?#{encoded_options}")
    assert_equal("application/json", response.content_type)
    assert_equal("XXX", JSON.parse(response.body))
  end

  private
  def groonga_suggest_httpd
    @groonga_suggest_httpd ||= guess_groonga_suggest_httpd_path
  end

  def groonga_suggest_create_dataset
    @groonga_suggest_create_dataset ||= guess_groonga_suggest_create_dataset_path
  end

  def guess_top_suggest_source_dir
    File.join(guess_top_source_dir, "src", "suggest")
  end

  def guess_groonga_suggest_httpd_path
    httpd = ENV["GROONGA_SUGGEST_HTTPD"]
    httpd ||= File.join(guess_top_suggest_source_dir, "groonga-suggest-httpd")
    File.expand_path(httpd)
  end

  def guess_groonga_suggest_create_dataset_path
    create_dataset = ENV["GROONGA_SUGGEST_CREATE_DATASET"]
    create_dataset ||= File.join(guess_top_suggest_source_dir,
                                 "groonga-suggest-create-dataset")
    File.expand_path(create_dataset)
  end

  def start_httpd
    command_line = [
      groonga_suggest_httpd,
      "--bind-address", @bind_address,
      "--port", @port.to_s,
      "--disable-max-fd-check",
      @database_path,
    ]
    @groonga_suggest_httpd_pid = start_server_process(@bind_address, @port,
                                                      *command_line)
  end
end
