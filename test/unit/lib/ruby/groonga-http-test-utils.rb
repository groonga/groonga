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

require 'groonga-test-utils'

require 'net/http'

gem 'json'
require 'json'

module GroongaHTTPTestUtils
  include GroongaTestUtils

  def setup_server
    super("http")
  end

  def teardown_server
    begin
      shutdown_server if @groonga_pid
    rescue Timeout::Error
    end
    super
  end

  private
  def shutdown_server
    get(command_path("shutdown"))
  end

  def get(path)
    Net::HTTP.start(@address, @port) do |http|
      http.get(path)
    end
  end

  def encode_options(options)
    return "" if options.empty?

    options.collect do |key, value|
      if value.nil?
        nil
      else
        "#{key}=#{URI.escape(value.to_s)}"
      end
    end.compact.join("&")
  end

  def command_path(command, options={})
    path = "/d/#{command}"
    encoded_options = encode_options(options)
    path += "?#{encoded_options}" unless encoded_options.empty?
    path
  end

  def populate_users
    create_users_table
    load_users
  end

  def create_users_table
    response = get(command_path(:table_create,
                                :name => "users",
                                :flags => Table::PAT_KEY,
                                :key_type => "ShortText"))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "application/json")

    response = get(command_path(:column_create,
                                :table => "users",
                                :name => "real_name",
                                :flags => Column::SCALAR,
                                :type => "ShortText"))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "application/json")

    response = get(command_path(:table_create,
                                :name => "terms",
                                :flags => Table::PAT_KEY,
                                :key_type => "ShortText",
                                :default_tokenizer => "TokenBigram"))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "application/json")

    response = get(command_path(:column_create,
                                :table => "terms",
                                :name => "users_real_name",
                                :flags => Column::INDEX,
                                :type => "users",
                                :source => "real_name"))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "application/json")
  end

  def load(table, values)
    n_values = values.size
    n_values -= 1 if values.first.is_a?(Array)
    response = get(command_path(:load,
                                :table => table,
                                :values => json(values)))
    assert_response([[Result::SUCCESS], n_values], response,
                    :content_type => "application/json")
  end

  def load_users
    load("users",
         [{:_key => "ryoqun", :real_name => "Ryo Onodera"},
          {:_key => "hayamiz", :real_name => "Yuto Hayamizu"}])
  end

  def create_bookmarks_table
    response = get(command_path(:table_create,
                                :name => "bookmarks",
                                :flags => Table::HASH_KEY,
                                :key_type => "Int32"))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "application/json")
  end

  def load_bookmarks(keys=nil)
    header = ["_key"]
    keys ||= (0...10).to_a

    load("bookmarks", [header, *keys.collect {|key| [key]}])

    id = 0
    keys.collect do |key|
      id += 1
      [id, key]
    end
  end

  def create_comments_table
    response = get(command_path(:table_create,
                                :name => "comments",
                                :flags => Table::NO_KEY))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "application/json")

    response = get(command_path(:column_create,
                                :table => "comments",
                                :name => "text",
                                :flags => Column::SCALAR,
                                :type => "ShortText"))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "application/json")

    response = get(command_path(:column_create,
                                :table => "comments",
                                :name => "author",
                                :flags => Column::SCALAR,
                                :type => "users"))
    assert_response([[Result::SUCCESS]], response,
                    :content_type => "application/json")
  end

  def load_comments
    load("comments",
         [[:text, :author],
          ["Ruby rocks", "ryoqun"],
          ["Groonga rocks", "hayamiz"]])
  end

  def json(object)
    JSON.generate(object)
  end

  def assert_response(expected, response, options=nil)
    actual = nil
    options ||= {}

    if options[:content_type]
      assert_equal(options[:content_type], response.content_type)
    end

    case response.content_type
    when "application/json"
      actual = JSON.parse(response.body)
    when "text/html"
      actual = response.body
    else
      flunk("unknown content-type: #{response.content_type}")
    end

    actual = yield(actual) if block_given?
    assert_equal(expected, actual)
  end

  def assert_select(header, expected, parameters, options={}, &block)
    response = get(command_path(:select, parameters))
    drilldown_records = options[:expected_drilldown] || []

    assert_response([[Result::SUCCESS],
                     [[options[:n_hits] || expected.size],
                      header,
                      *expected
                     ],
                     *drilldown_records],
                    response,
                    :content_type => "application/json",
                    &block)
  end
end
