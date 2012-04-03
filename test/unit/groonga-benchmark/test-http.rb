# -*- coding: utf-8 -*-
#
# Copyright (C) 2010-2011  Kouhei Sutou <kou@clear-code.com>
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

class GroongaBenchmarkHTTPTest < Test::Unit::TestCase
  include GroongaBenchmarkTestUtils

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
    output, error, status = invoke_groonga_benchmark("--groonga", groonga,
                                           "--protocol", "http",
                                           "--port", "20041",
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

  def test_report_http_json
    command = tempfile("command") do |file|
      file.puts('select Shops --sortby _id --limit 5 --output_columns "name"')
    end
    groonga_benchmark_command = "rep_http #{command.path}"
    script = tempfile("script") do |file|
      file.puts(groonga_benchmark_command)
    end
    log = tempfile("log")
    output, error, status = invoke_groonga_benchmark("--groonga", groonga,
                                                     "--protocol", "http",
                                                     "--port", "20041",
                                                     "--log-path", log.path,
                                                     script.path, @database_path)
    assert_predicate(status, :success?, [output, error])
    jobs_list = JSON.parse(log.read).find_all do |element|
      element.has_key?("jobs")
    end
    assert_equal([groonga_benchmark_command],
                 jobs_list.collect {|jobs| jobs["jobs"]})
  end

  def test_report_http_xml
    command = tempfile("command") do |file|
      file.puts('select Shops --sortby _id --limit 5 --output_columns "name" ' +
                '--output_type xml')
    end
    groonga_benchmark_command = "rep_http #{command.path}"
    script = tempfile("script") do |file|
      file.puts(groonga_benchmark_command)
    end
    log = tempfile("log")
    output, error, status = invoke_groonga_benchmark("--groonga", groonga,
                                                     "--protocol", "http",
                                                     "--port", "20041",
                                                     "--log-path", log.path,
                                                     script.path, @database_path)
    assert_predicate(status, :success?, [output, error])
    jobs_list = JSON.parse(log.read).find_all do |element|
      element.has_key?("jobs")
    end
    assert_equal([groonga_benchmark_command],
                 jobs_list.collect {|jobs| jobs["jobs"]})
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
    output, error, status = invoke_groonga_benchmark("--groonga", groonga,
                                                     "--protocol", "http",
                                                     "--port", "20041",
                                                     "--log-output-dir", @tmp_dir,
                                                     script_file.path,
                                                     @database_path)
    assert_predicate(status, :success?, [output, error])
    assert_equal("", File.read("#{expected_file.path}.diff"))
  end

  def test_test_http_diff
    command = 'select Shops --sortby _id --limit 5 --output_columns "name"'
    command_file = tempfile("command") do |file|
      file.puts(command)
    end
    expected =
      '[[0,1290511592.67556,0.00068249],' +
      '[[[36],' +
      '[["name","ShortText"]],' +
      '["たい焼 カタオカ"],' +
      '["根津のたいやき"],' +
      '["そばたいやき空"],' +
      '["車"],' +
      '["広瀬屋"]]]]'
    actual =
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
    output, error, status = invoke_groonga_benchmark("--groonga", groonga,
                                                     "--protocol", "http",
                                                     "--port", "20041",
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
