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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

class BOMTest < Test::Unit::TestCase
  include GroongaTestUtils

  def setup
    setup_database_path
    @input_file = File.join(@tmp_dir, "commands")
  end

  def teardown
    teardown_database_path
  end

  def test_no_bom
    open(@input_file, "w") do |file|
      file.puts("defrag")
    end
    assert_equal("[[0,0.0,0.0],true]\n",
                 run_groonga("--file", @input_file, "-n", @database_path))
  end

  def test_bom
    open(@input_file, "w") do |file|
      file.print("\xef\xbb\xbf")
      file.puts("defrag")
    end
    assert_equal("[[0,0.0,0.0],true]\n",
                 run_groonga("--file", @input_file, "-n", @database_path))
  end
end
