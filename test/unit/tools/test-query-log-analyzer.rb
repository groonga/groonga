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
require "stringio"

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

  class SelectCommandParseTest < Test::Unit::TestCase
    include CommandParseTestUtils

    def test_parameters
      select = parse("/d/select.json",
                     :table => "Users",
                     :filter => "age<=30")
      assert_equal(command("select",
                           "table" => "Users",
                           "filter" => "age<=30",
                           "output_type" => "json"),
                   select)
    end

    def test_scorer
      select = parse("/d/select.json",
                     :table => "Users",
                     :filter => "age<=30",
                     :scorer => "_score = random()")
      assert_equal("_score = random()", select.scorer)
    end
  end

  class SelectCommandParseFilterTest < Test::Unit::TestCase
    include CommandParseTestUtils

    def test_parenthesis
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

  class StatisticStepParseTest < Test::Unit::TestCase
    def setup
      @parser = GroongaQueryLogAnaylzer::QueryLogParser.new
    end

    def test_name
      @parser.parse(StringIO.new(log))
      steps = []
      @parser.statistics.first.each_step do |step|
        steps << [step[:name], step[:context]]
      end
      expected = [
        ["filter", "local_name @ \"gsub\""],
        ["filter", "description @ \"string\""],
        ["select", nil],
        ["sort", "_score"],
        ["output", "_key"],
      ]
      assert_equal(expected, steps)
    end

    private
    def log
      <<-EOL
2011-06-02 14:28:52.951973|28027ba0|>/d/select.join?table=Entries&filter=local_name+%40+%22gsub%22+%26%26+description+%40+%22string%22&sortby=_score&output_columns=_key
2011-06-02 14:28:52.953499|28027ba0|:000000001523131 filter(15)
2011-06-02 14:28:52.954335|28027ba0|:000000002362555 filter(13)
2011-06-02 14:28:52.954381|28027ba0|:000000002408669 select(13)
2011-06-02 14:28:52.954528|28027ba0|:000000002555263 sort(10)
2011-06-02 14:28:52.954821|28027ba0|:000000002847858 output(10)
2011-06-02 14:28:52.954958|28027ba0|<000000002985581 rc=0
EOL
    end
  end
end
