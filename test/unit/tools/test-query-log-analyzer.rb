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

require "groonga-query-log-analyzer"

module QueryLogAalyzerTest
  module CommandParseTestUtils
    private
    def command(name, parameters)
      GroongaQueryLogAnaylzer::Command.new(name, parameters)
    end

    def parse(command_path, parameters)
      path = "#{command_path}?"
      path << parameters.collect do |key, value|
        [CGI.escape(key.to_s), CGI.escape(value.to_s)].join("=")
      end.join("&")
      GroongaQueryLogAnaylzer::Command.parse(path)
    end
  end

  class CommandParseTest < Test::Unit::TestCase
    include CommandParseTestUtils

    def test_simple
      select = parse("/d/select.json",
                     :table => "Users",
                     :filter => "age<=30")
      assert_equal(command("select",
                           "table" => "Users",
                           "filter" => "age<=30",
                           "output_type" => "json"),
                   select)
    end
  end

  class SelectCommandFilterParseTest < Test::Unit::TestCase
    include CommandParseTestUtils

    def test_simple
      filter = 'geo_in_rectangle(location,' +
                                '"35.73360x139.7394","62614x139.7714") && ' +
               '((type == "たいやき" || type == "和菓子")) && ' +
               'keyword @ "たいやき" &! keyword @ "白" &! keyword @ "養殖"'
      select = parse("/d/select.json",
                     :table => "Users",
                     :filter => filter)
      assert_equal(['geo_in_rectangle(location,' +
                                     '"35.73360x139.7394","62614x139.7714")',
                     'type == "たいやき"',
                     'type == "和菓子"',
                     'keyword @ "たいやき"',
                     'keyword @ "白"',
                     'keyword @ "養殖"'],
                   select.conditions)
    end
  end
end
