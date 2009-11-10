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

require 'net/http'

gem 'json'
require 'json'

module GroongaHTTPTestUtils
  include GroongaConstants

  private
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
    assert_equal("true", response.body)

    response = get(command_path(:column_create,
                                :table => "users",
                                :name => "real_name",
                                :flags => Column::SCALAR,
                                :type => "ShortText"))
    assert_equal("true", response.body)

    response = get(command_path(:table_create,
                                :name => "terms",
                                :flags => Table::PAT_KEY,
                                :key_type => "ShortText",
                                :default_tokenizer => "TokenBigram"))
    assert_equal("true", response.body)

    response = get(command_path(:column_create,
                                :table => "terms",
                                :name => "users_real_name",
                                :flags => Column::INDEX,
                                :type => "users",
                                :source => "real_name"))
    assert_equal("true", response.body)
  end

  def load_users
    values = json([{:_key => "ryoqun", :real_name => "Ryo Onodera"}])
    response = get(command_path(:load, :table => "users", :values => values))
    assert_equal("1", response.body)

    values = json([{:_key => "hayamiz", :real_name => "Yuto Hayamizu"}])
    response = get(command_path(:load, :table => "users", :values => values))
    assert_equal("1", response.body)
  end

  def json(object)
    JSON.generate(object)
  end
end
