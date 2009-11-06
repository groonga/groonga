# -*- coding: utf-8 -*-
#
# Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>
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

require 'fileutils'
require 'net/http'

gem 'json'
require 'json'

module GroongaTestUtils
  def setup_server
    @groonga = guess_groonga_path
    @tmp_dir = File.join(File.dirname(__FILE__), "tmp")
    FileUtils.rm_rf(@tmp_dir)
    FileUtils.mkdir_p(@tmp_dir)
    @database_path = File.join(@tmp_dir, "database")
    @address = "127.0.0.1"
    @port = 5454
    @encoding = "utf8"
    @groonga_pid = fork do
      exec(@groonga,
           "-s",
           "-i", @address,
           "-p", @port.to_s,
           "-n", @database_path,
           "-e", @encoding)
    end
    sleep 1
  end

  def teardown_server
    Process.kill(:TERM, @groonga_pid)
    begin
      Process.waitpid(@groonga_pid)
    rescue Errno::ECHILD
    end
    FileUtils.rm_r(@tmp_dir)
  end

  private
  def guess_groonga_path
    groonga = ENV["GROONGA"]
    groonga ||= File.join(File.dirname(__FILE__),
                          "..", "..", "..", "..", "..",
                          "src", "groonga")
    groonga
  end
end
