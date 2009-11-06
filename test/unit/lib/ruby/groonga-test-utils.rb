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

module GroongaTestUtils
  def setup_server
    @groonga = guess_groonga_path
    @resource_dir = guess_resource_dir
    @tmp_dir = File.join(File.dirname(__FILE__), "tmp")
    FileUtils.rm_rf(@tmp_dir)
    FileUtils.mkdir_p(@tmp_dir)
    @database_path = File.join(@tmp_dir, "database")
    @address = "127.0.0.1"
    @port = 5454
    @encoding = "utf8"
    start_server
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
    File.expand_path(groonga)
  end

  def guess_resource_dir
    File.expand_path(File.join(File.dirname(@groonga), "..",
                               "resource", "admin_html"))
  end

  def start_server
    @groonga_pid = fork do
      exec(@groonga,
           "-s",
           "-i", @address,
           "-p", @port.to_s,
           "-e", @encoding,
           "--admin-html-path", @resource_dir,
           "-n", @database_path)
    end

    Timeout.timeout(1) do
      loop do
        begin
          TCPSocket.new(@address, @port)
          break
        rescue SystemCallError
        end
      end
    end
  end
end
