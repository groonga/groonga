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
require 'shellwords'
require 'tmpdir'
require 'groonga-constants'

module GroongaTestUtils
  include GroongaConstants

  def setup_database_path
    @tmp_dir = Dir.mktmpdir("tmp", ENV["BUILD_DIR"])
    FileUtils.rm_rf(@tmp_dir)
    FileUtils.mkdir_p(@tmp_dir)
    @database_path = File.join(@tmp_dir, "database")
  end

  def teardown_database_path
    @tmp_dir ||= nil
    FileUtils.rm_rf(@tmp_dir) if @tmp_dir
  end

  def setup_server(protocol=nil)
    setup_database_path
    @protocol = protocol
    @groonga = guess_groonga_path
    @resource_dir = guess_resource_dir
    @address = "127.0.0.1"
    @port = 5454
    @encoding = "utf8"
    @user_object_start_id = 256
    start_server
  end

  def teardown_server
    @groonga_pid ||= nil
    if @groonga_pid
      Process.kill(:TERM, @groonga_pid)
      begin
        Process.waitpid(@groonga_pid)
      rescue Errno::ECHILD
      end
      @groonga_pid = nil
    end

    teardown_database_path
  end

  private
  def guess_groonga_path
    groonga = ENV["GROONGA"]
    groonga ||= File.join(guess_top_source_dir, "src", "groonga")
    File.expand_path(groonga)
  end

  def guess_resource_dir
    File.join(guess_top_source_dir, "resource", "admin_html")
  end

  def guess_top_source_dir
    base_dir = ENV["BASE_DIR"]
    if base_dir
      top_source_dir = File.join(base_dir, "..", "..")
    else
      top_source_dir = File.join(File.dirname(__FILE__), "..", "..", "..", "..")
    end
    File.expand_path(top_source_dir)
  end

  def start_server
    arguments = ["-s",
                 "-a", @address,
                 "-p", @port.to_s,
                 "-e", @encoding,
                 "--admin-html-path", @resource_dir]
    arguments.concat(["--protocol", @protocol]) if @protocol
    arguments.concat(["-n", @database_path])
    @groonga_pid = fork do
      exec(@groonga, *arguments)
    end

    sleep 0.3 # wait for groonga server initialize

    begin
      timeout(1) do
        loop do
          begin
            TCPSocket.new(@address, @port)
            break
          rescue SystemCallError
          end
        end
      end
    rescue
      @groonga_pid = nil
      raise
    end
  end

  def object_registered
    current_id = @user_object_start_id
    @user_object_start_id += 1
    current_id
  end

  def timeout(seconds, &block)
    Timeout.timeout(seconds, &block)
  rescue Timeout::Error
    raise RuntimeError, "timeout #{seconds}s", [caller[0]] + $@
  end

  def construct_command_line(*arguments)
    command_line = [guess_groonga_path, *arguments].collect do |component|
      Shellwords.escape(component)
    end.join(" ")
  end

  def run_groonga(*arguments)
    `#{construct_command_line(*arguments)}`
  end

  def utf8(string)
    string.force_encoding("UTF-8") if string.respond_to?(:force_encoding)
    string
  end

  LANG_ENVS = %w"LANG LC_ALL LC_CTYPE"

  def invoke_groonga(args, stdin_data="", capture_stdout=false, capture_stderr=false, opt={})
    @groonga ||= guess_groonga_path
    args = [args] unless args.kind_of?(Array)
    begin
      in_child, in_parent = IO.pipe
      out_parent, out_child = IO.pipe if capture_stdout
      err_parent, err_child = IO.pipe if capture_stderr
      pid = fork do
        c = "C"
        LANG_ENVS.each {|lc| ENV[lc] = c}
        case args.first
        when Hash
          ENV.update(args.shift)
        end
        STDIN.reopen(in_child)
        in_parent.close
        if capture_stdout
          STDOUT.reopen(out_child)
          out_parent.close
        end
        if capture_stderr
          STDERR.reopen(err_child)
          err_parent.close
        end
        Process.setrlimit(Process::RLIMIT_CORE, 0) rescue nil
        exec(@groonga, *args)
      end
      in_child.close
      out_child.close if capture_stdout
      err_child.close if capture_stderr
      th_stdout = Thread.new { out_parent.read } if capture_stdout
      th_stderr = Thread.new { err_parent.read } if capture_stderr
      in_parent.write stdin_data.to_str
      in_parent.close
      if (!capture_stdout || th_stdout.join(10)) && (!capture_stderr || th_stderr.join(10))
        stdout = th_stdout.value if capture_stdout
        stderr = th_stderr.value if capture_stderr
      else
        raise Timeout::Error
      end
      Process.wait pid
      status = $?
    ensure
      [in_child, in_parent, out_child, out_parent, err_child, err_parent].each do |io|
        io.close if io && !io.closed?
      end
      [th_stdout, th_stderr].each do |th|
        (th.kill; th.join) if th
      end
    end
    return stdout, stderr, status
  end

  def assert_run_groonga(test_stdout, test_stderr, args, *rest)
    argnum = rest.size + 3
    opt = (Hash === rest.last ? rest.pop : {})
    message = (rest.pop if String === rest.last)
    if String === rest.last
      stdin = rest.pop
    else
      stdin = opt.delete(:stdin) || ""
    end
    unless rest.empty?
      raise ArgumentError, "wrong number of arguments (#{argnum} for 3)"
    end
    stdout, stderr, status = invoke_groonga(args, stdin, true, true, opt)
    assert_not_predicate(status, :signaled?)
    if block_given?
      yield(stdout, stderr)
    else
      if test_stderr.is_a?(Regexp)
        assert_match(test_stderr, stderr, message)
      else
        assert_equal(test_stderr, stderr, message)
      end
      if test_stdout.is_a?(Regexp)
        assert_match(test_stdout, stdout, message)
      else
        assert_equal(test_stdout, stdout, message)
      end
      status
    end
  end
end
