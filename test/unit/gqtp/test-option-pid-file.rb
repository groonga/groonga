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

class OptionPidFileTest < Test::Unit::TestCase
  include GroongaLocalGQTPTestUtils

  def setup
    #setup_local_database
  end

  def teardown
    #teardown_local_database
  end

  def test_daemon_pid_file
    require 'tmpdir'
    tmpdir = Dir.mktmpdir
    pidfile = File.join(tmpdir, "groonga.pid")
    output = run_groonga("-d", "--pid-file", pidfile)
    assert_equal("", output)
    pid = open(pidfile) do |f|
      assert(f.stat.file?)
      Integer(f.read)
    end
    assert_equal(1, Process.kill(0, pid))
    sleep 1
    run_groonga("-c", "localhost", "shutdown")
    assert_raise(Errno::ESRCH) do
      while Process.kill(0, pid)
        sleep 0.1
      end
    end
  ensure
    FileUtils.rm_rf(tmpdir)
  end
end
