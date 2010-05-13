# -*- coding: utf-8 -*-
#
# Copyright (C) 2009-2010  Kouhei Sutou <kou@clear-code.com>
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
  rescue SystemCallError
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
    options = options.dup
    output_type = options.delete(:output_type)
    path += ".#{output_type}" if output_type
    encoded_options = encode_options(options)
    path += "?#{encoded_options}" unless encoded_options.empty?
    path
  end

  def populate_users
    create_users_table
    load_users
  end

  def populate_tags
    create_users_table
    load_tags
  end

  def table_create(name, options={})
    response = get(command_path(:table_create,
                                options.merge({:name => name})))
    assert_success_response(response, :content_type => "application/json")
    object_registered
  end

  def column_create(table, name, flags, type, options={})
    response = get(command_path(:column_create,
                                options.merge(:table => table,
                                              :name => name,
                                              :flags => flags,
                                              :type => type)))
    assert_success_response(response, :content_type => "application/json")
    object_registered
  end

  def view_add(view, table)
    response = get(command_path(:view_add,
                                :view => view,
                                :table => table))
    assert_success_response(response, :content_type => "application/json")
  end

  def create_users_table
    table_create("users",
                 :flags => Table::PAT_KEY,
                 :key_type => "ShortText")
    column_create("users", "real_name", Column::SCALAR, "ShortText")
    column_create("users", "hp", Column::SCALAR, "Int32")

    table_create("terms",
                 :flags => Table::PAT_KEY,
                 :key_type => "ShortText",
                 :default_tokenizer => "TokenBigram")
    column_create("terms", "users_real_name", Column::INDEX, "users",
                  :source => "real_name")

    table_create("tags",
                 :flags => Table::HASH_KEY,
                 :key_type => "ShortText")
  end

  def load(table, values)
    n_values = values.size
    n_values -= 1 if values.first.is_a?(Array)
    response = get(command_path(:load,
                                :table => table,
                                :values => json(values)))
    assert_response([success_status_response, n_values], response,
                    :content_type => "application/json")
  end

  def load_users
    load("users",
         [{:_key => "ryoqun", :real_name => "Ryo Onodera", :hp => 200},
          {:_key => "hayamiz", :real_name => "Yuto Hayamizu", :hp => 200}])
  end

  def load_many_users
    load("users",
         [{:_key => "moritan", :real_name => "モリタン", :hp => 100},
          {:_key => "taporobo", :real_name => "タポロボ", :hp => 100},
          {:_key => "ryoqun", :real_name => "Ryo Onodera", :hp => 200},
          {:_key => "hayamiz", :real_name => "Yuto Hayamizu", :hp => 200},
          {:_key => "gunyara-kun", :real_name => "Tasuku SUENAGA", :hp => 150}])
  end

  def load_tags
    load("tags",
         [{:_key => "programmer"},
          {:_key => "CEO"},
          {:_key => "male"}])
  end

  def create_calendar_table
    table_create("calendar", :flags => Table::NO_KEY)
    column_create("calendar", "month", Column::SCALAR, "Int32")
    column_create("calendar", "day", Column::SCALAR, "Int32")
  end

  def load_schedules
    header = ["month", "day"]

    records = []
    1.upto(12) do |month|
      days = (1..28).to_a.shuffle
      1.upto(10) do
        records.push([month, days.pop])
      end
    end
    records.shuffle!

    load("calendar", [header, *records])

    id = 0
    records.collect do |record|
      id += 1
      [id , *record]
    end
  end

  def json(object)
    JSON.generate(object)
  end

  def success_status_response
    [Result::SUCCESS, 0.0, 0.0]
  end

  def assert_response(expected, response, options=nil)
    actual = nil
    options ||= {}

    if options[:content_type]
      assert_equal(options[:content_type], response.content_type)
    end

    case response.content_type
    when "application/json"
      begin
        actual = JSON.parse(response.body)
      rescue JSON::ParserError => e
        raise "JSON ParserError #{e.message}\nJSON is ...\n" \
              "---\n#{response.body}\n---"
      end
      if actual[0][0].is_a?(Integer)
        actual[0][1..2] = [0.0, 0.0]
        actual[0][4] = nil if actual[0][4]
      end
    when "text/html"
      actual = response.body
    when "text/xml"
      actual = response.body
    else
      flunk("unknown content-type: #{response.content_type}")
    end

    actual = yield(actual) if block_given?
    assert_equal(expected, actual)
  end

  def assert_success_response(response, options=nil)
    assert_response([success_status_response], response, options)
  end

  def assert_response_body(body, response, options=nil, &block)
    assert_response([success_status_response, body], response, options, &block)
  end

  def assert_error_response(code, message, response, options=nil)
    assert_response([[code, 0.0, 0.0, message, nil]], response, options)
  end

  def assert_select(header, expected, parameters, options={}, &block)
    command_name = options[:command] || :select
    response = get(command_path(command_name, parameters))
    drilldown_results = options[:drilldown_results] || []

    assert_response([success_status_response,
                     [[[options[:n_hits] || expected.size],
                       header,
                       *expected
                      ],
                     *drilldown_results]],
                    response,
                    :content_type => "application/json",
                    &block)
  end

  def assert_select_xml(expected, parameters, options={}, &block)
    command_name = options[:command] || :select
    response = get(command_path(command_name,
                                parameters.merge(:output_type => "xml")))

    assert_response(expected,
                    response,
                    :content_type => "text/xml",
                    &block)
  end
end
