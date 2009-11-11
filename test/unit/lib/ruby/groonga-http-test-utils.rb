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

  def teardown_server
    begin
      shutdown_server
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
    response = get(command_path(:load,
                                :table => table,
                                :values => json(values)))
    assert_response([[Result::SUCCESS], values.size], response,
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

  def load_bookmarks
    values = [["_key"]]
    expected = [[Result::SUCCESS]]
    expected_records = []
    id = 1
    (0...10).to_a.each do |number|
      values.push([number])
      expected_records.push([id, number])
      id += 1
    end
    expected_records.sort! {|record1, record2| record1[1] <=> record2[1]}
    expected_records = [[10], ["_id", "_key"]] + expected_records
    expected.push(expected_records)

    response = get(command_path(:load,
                                :table => "bookmarks",
                                :values => json(values)))
    assert_response([[Result::SUCCESS], values.size-1], response,
                    :content_type => "application/json")
    expected
  end

  def load_shuffled_bookmarks
    srand(Time.now.to_i)
    values = [["_key"]]
    expected = [[Result::SUCCESS]]
    expected_records = []
    id = 1
    (0...10).to_a.shuffle.each do |number|
      values.push([number])
      expected_records.push([id, number])
      id += 1
    end
    expected_records.sort! {|record1, record2| record1[1] <=> record2[1]}
    expected_records = [[10], ["_id", "_key"]] + expected_records
    expected.push(expected_records)
    response = get(command_path(:load,
                                :table => "bookmarks",
                                :values => json(values)))
    assert_response([[Result::SUCCESS], values.size-1], response,
                    :content_type => "application/json")
    expected
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
end
