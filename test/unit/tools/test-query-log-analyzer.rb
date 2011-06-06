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

require "groonga-query-log-analyzer"
require "stringio"

module QueryLogAalyzerTest
  module CommandParseTestUtils
    private
    def command(name, parameters)
      GroongaQueryLogAnaylzer::Command.new(name, parameters)
    end

    def parse_http_path(command, parameters)
      path = "/d/#{command}.json?"
      path << parameters.collect do |key, value|
        [CGI.escape(key.to_s), CGI.escape(value.to_s)].join("=")
      end.join("&")
      GroongaQueryLogAnaylzer::Command.parse(path)
    end

    def parse_command_line(command, parameters)
      command_line = "#{command} --output_type json"
      parameters.each do |key, value|
        if /"| / =~ value
          escaped_value = '"' + value.gsub(/"/, '\"') + '"'
        else
          escaped_value = value
        end
        command_line << " --#{key} #{escaped_value}"
      end
      GroongaQueryLogAnaylzer::Command.parse(command_line)
    end
  end

  module HTTPCommandParseTestUtils
    private
    def parse(command, parameters)
      parse_http_path(command, parameters)
    end
  end

  module CommandLineCommandParseTestUtils
    private
    def parse(command, parameters)
      parse_command_line(command, parameters)
    end
  end

  module SelectCommandParseTests
    include CommandParseTestUtils

    def test_parameters
      select = parse("select",
                     :table => "Users",
                     :filter => "age<=30")
      assert_equal(command("select",
                           "table" => "Users",
                           "filter" => "age<=30",
                           "output_type" => "json"),
                   select)
    end

    def test_scorer
      select = parse("select",
                     :table => "Users",
                     :filter => "age<=30",
                     :scorer => "_score = random()")
      assert_equal("_score = random()", select.scorer)
    end
  end

  module SelectCommandParseFilterTests
    include CommandParseTestUtils

    def test_parenthesis
      filter = 'geo_in_rectangle(location,' +
                                '"35.73360x139.7394","62614x139.7714") && ' +
               '((type == "たいやき" || type == "和菓子")) && ' +
               'keyword @ "たいやき" &! keyword @ "白" &! keyword @ "養殖"'
      select = parse("select",
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

  class HTTPSelectCommandParseTest < Test::Unit::TestCase
    include SelectCommandParseTests
    include SelectCommandParseFilterTests
    include HTTPCommandParseTestUtils
  end

  class CommandLineSelecCommandParseTest < Test::Unit::TestCase
    include SelectCommandParseTests
    include SelectCommandParseFilterTests
    include CommandLineCommandParseTestUtils
  end

  class StatisticStepParseTest < Test::Unit::TestCase
    def test_context
      steps = statistics.first.steps.collect do |step|
        [step[:name], step[:context]]
      end
      expected = [
        ["filter", "local_name @ \"gsub\""],
        ["filter", "description @ \"string\""],
        ["select", nil],
        ["sort", "_score"],
        ["output", "_key"],
        ["drilldown", "name"],
        ["drilldown", "class"],
      ]
      assert_equal(expected, steps)
    end

    def test_n_records
      steps = statistics.first.steps.collect do |step|
        [step[:name], step[:n_records]]
      end
      expected = [
        ["filter", 15],
        ["filter", 13],
        ["select", 13],
        ["sort", 10],
        ["output", 10],
        ["drilldown", 3],
        ["drilldown", 2],
      ]
      assert_equal(expected, steps)
    end

    private
    def log
      <<-EOL
2011-06-02 16:27:04.731685|5091e5c0|>/d/select.join?table=Entries&filter=local_name+%40+%22gsub%22+%26%26+description+%40+%22string%22&sortby=_score&output_columns=_key&drilldown=name,class
2011-06-02 16:27:04.733539|5091e5c0|:000000001849451 filter(15)
2011-06-02 16:27:04.734978|5091e5c0|:000000003293459 filter(13)
2011-06-02 16:27:04.735012|5091e5c0|:000000003327415 select(13)
2011-06-02 16:27:04.735096|5091e5c0|:000000003411824 sort(10)
2011-06-02 16:27:04.735232|5091e5c0|:000000003547265 output(10)
2011-06-02 16:27:04.735606|5091e5c0|:000000003921419 drilldown(3)
2011-06-02 16:27:04.735762|5091e5c0|:000000004077552 drilldown(2)
2011-06-02 16:27:04.735808|5091e5c0|<000000004123726 rc=0
EOL
    end

    def statistics
      statistics = GroongaQueryLogAnaylzer::SizedStatistics.new(100, "-elapsed")
      parser = GroongaQueryLogAnaylzer::QueryLogParser.new(statistics)
      parser.parse(StringIO.new(log))
      statistics
    end
  end
end
