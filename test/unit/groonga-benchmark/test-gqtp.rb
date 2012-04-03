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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

class GroongaBenchmarkGQTPTest < Test::Unit::TestCase
  include GroongaBenchmarkTestUtils

  CONFIG_ENV = {"GRN_CONFIG_PATH" => ""}

  def setup
    setup_database
  end

  def teardown
    teardown_database
  end

  def test_do_multi_thread
    command = 'select Shops --sortby _id --limit 5 --output_columns "name"'
    command_file = tempfile("command") do |file|
      file.puts(command)
    end
    script_file = tempfile("script") do |file|
      file.puts("do_gqtp #{command_file.path} 10 5")
      file.puts("do_gqtp #{command_file.path} 4 2")
    end
    output, error, status = invoke_groonga_benchmark("--groonga", groonga,
                                                     "--protocol", "gqtp",
                                                     "--port", "20041",
                                                     "--log-output-dir", @tmp_dir,
                                                     script_file.path,
                                                     @database_path)
    assert_predicate(status, :success?, [output, error])
    log_file = nil
    Dir.glob("#{@tmp_dir}/*.log") do |file|
      log_file = file
    end
    result = JSON.parse(File.read(log_file))
    assert_equal((10 * 5) + (4 * 2), result.last["queries"])
  end
end
