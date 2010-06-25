# -*- coding: utf-8 -*-
#
# Copyright (C) 2010  Nobuyoshi Nakada <nakada@clear-code.com>
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

class OptionTest < Test::Unit::TestCase
  include GroongaTestUtils

  def setup
    setup_database_path
  end

  def teardown
    teardown_database_path
  end

  def test_daemon_pid_file
    pid_file = File.join(@tmp_dir, "groonga.pid")
    assert_path_not_exist(pid_file)
    assert_equal("", run_groonga("-d", "--pid-file", pid_file))
    assert_path_exist(pid_file)
    pid = File.open(pid_file) do |f|
      Integer(f.read)
    end
    assert_equal(1, Process.kill(:INT, pid))
    30.times do
      break unless File.exist?(pid_file)
      sleep 0.1
    end
    assert_path_not_exist(pid_file)
  end

  def test_help_option
    usage = run_groonga("--help")
    assert_predicate($?, :success?)
    assert_match(/\AUsage: groonga \[options\.\.\.\] \[dest\]$/, usage)
  end
end
