# -*- coding: utf-8 -*-
#
# Copyright (C) 2009  Ryo Onodera <onodera@clear-code.com>
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

class DumpTest < Test::Unit::TestCase
  include GroongaTestUtils

  def setup
    @groonga = guess_groonga_path

    @tmp_dir = File.join(File.dirname(__FILE__), "tmp")
    FileUtils.rm_rf(@tmp_dir)
    FileUtils.mkdir_p(@tmp_dir)
    @database_path = File.join(@tmp_dir, "database")

    @pipe = IO.popen([@groonga, "-n", @database_path].join(' '), "w+")
  end

  def teardown
    @pipe.close
    FileUtils.rm_rf(@tmp_dir) if @tmp_dir
  end

  def puts(*args)
    timeout(3) do
      @pipe.puts(*args)
    end
  end

  def gets(*args)
    timeout(3) do
      @pipe.gets(*args)
    end
  end

  def test_dump
    puts("dump")
    assert_equal("true\n", gets)
  end
end
