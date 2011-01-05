# -*- coding: utf-8 -*-
#
# Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
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

class GrnTestOutHTTPTest < Test::Unit::TestCase
  include GroongaGrnTestTestUtils

  CONFIG_ENV = {"GRN_CONFIG_PATH" => ""}

  def setup
    setup_database
  end

  def teardown
    teardown_database
  end

  def test_out_http
    command = tempfile("command") do |file|
      file.puts('select Shops --sortby _id --limit 5 --output_columns "name"')
    end
    expected = tempfile("expected")
    script = tempfile("script") do |file|
      file.puts("out_http #{command.path} #{expected.path}")
    end
    output, error, status = invoke_grntest("--noftp",
                                           "--groonga", groonga,
                                           "--protocol", "http",
                                           "--log-output-dir", @tmp_dir,
                                           script.path, @database_path)
    assert_predicate(status, :success?, [output, error])
    status, result = JSON.parse(expected.read)
    assert_equal([0,
                  [[[36],
                    [["name", "ShortText"]],
                    ["根津のたいやき"],
                    ["たい焼 カタオカ"],
                    ["そばたいやき空"],
                    ["車"],
                    ["広瀬屋"]]]],
                 [status[0], result])
  end

  def test_test_http_same
    command = 'select Shops --sortby _id --limit 5 --output_columns "name"'
    command_file = tempfile("command") do |file|
      file.puts(command)
    end
    expected =
      '[[0,1290511592.67556,0.00068249],' +
      '[[[36],' +
      '[["name","ShortText"]],' +
      '["根津のたいやき"],' +
      '["たい焼 カタオカ"],' +
      '["そばたいやき空"],' +
      '["車"],' +
      '["広瀬屋"]]]]'
    expected_file = tempfile("expected") do |file|
      file.puts(expected)
    end
    script_file = tempfile("script") do |file|
      file.puts("test_http #{command_file.path} #{expected_file.path}")
    end
    output, error, status = invoke_grntest("--noftp",
                                           "--groonga", groonga,
                                           "--protocol", "http",
                                           "--log-output-dir", @tmp_dir,
                                           script_file.path,
                                           @database_path)
    assert_predicate(status, :success?, [output, error])
    assert_equal("", File.read("#{expected_file.path}.diff"))
  end

  def test_test_http_diff
    command = 'select Shops --sortby _id --limit 3 --output_columns "name"'
    command_file = tempfile("command") do |file|
      file.puts(command)
    end
    expected =
      '[[0,1290511592.67556,0.00068249],' +
      '[[[36],' +
      '[["name","ShortText"]],' +
      '["根津のたいやき"],' +
      '["たい焼 カタオカ"],' +
      '["そばたいやき空"],' +
      '["車"],' +
      '["広瀬屋"]]]]'
    actual =
      '[[0,1290511592.67556,0.00068249],' +
      '[[[36],' +
      '[["name","ShortText"]],' +
      '["根津のたいやき"],' +
      '["たい焼 カタオカ"],' +
      '["そばたいやき空"]]]]'
    expected_file = tempfile("expected") do |file|
      file.puts(expected)
    end
    script_file = tempfile("script") do |file|
      file.puts("test_http #{command_file.path} #{expected_file.path}")
    end
    output, error, status = invoke_grntest("--noftp",
                                           "--groonga", groonga,
                                           "--protocol", "http",
                                           "--log-output-dir", @tmp_dir,
                                           script_file.path,
                                           @database_path)
    assert_predicate(status, :success?, [output, error])
    assert_equal("DIFF:command:#{command}\n" +
                 "DIFF:result:#{normalize_result(actual)}\n" +
                 "DIFF:expect:#{normalize_result(expected)}\n",
                 normalize_result(File.read("#{expected_file.path}.diff")))
  end

  private
  def normalize_result(result)
    result.gsub(/\[\[0,[\d.]+,[\d.]+\],/, '[[0,0.0,0.0],')
  end
end
