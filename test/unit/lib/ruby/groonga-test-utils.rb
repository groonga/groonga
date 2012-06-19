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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

require 'fileutils'
require 'shellwords'
require 'tmpdir'
require 'groonga-constants'

module GroongaTestUtils
  include GroongaConstants

  def setup_database_path
    base_dir = ENV["BUILD_DIR"] || ENV["BASE_DIR"]
    base_dir ||= File.join(File.dirname(__FILE__), "..", "..")
    @tmp_base_dir = File.join(File.expand_path(base_dir), "tmp")
    FileUtils.rm_rf(@tmp_base_dir)
    FileUtils.mkdir_p(@tmp_base_dir)
    @tmp_dir = Dir.mktmpdir("tmp", @tmp_base_dir)
    @database_path = File.join(@tmp_dir, "database")
  end

  def teardown_database_path
    @tmp_base_dir ||= nil
    FileUtils.rm_rf(@tmp_base_dir) if @tmp_base_dir
  end

  def setup_server(protocol=nil)
    setup_database_path
    @protocol = protocol
    @bind_address = "127.0.0.1"
    @port = 5454
    @encoding = "utf8"
    @user_object_start_id = 256
    start_server
  end

  def teardown_server
    @groonga_pid ||= nil
    stop_server_process(@groonga_pid)
    @groonga_pid = nil

    teardown_database_path
  end

  private
  def guess_groonga_path
    groonga = ENV["GROONGA"]
    groonga ||= File.join(guess_top_source_dir, "src", "groonga")
    File.expand_path(groonga)
  end

  def groonga
    @groonga ||= guess_groonga_path
  end

  def guess_document_root
    File.join(guess_top_source_dir, "data", "html", "admin")
  end

  def document_root
    @document_root ||= guess_document_root
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

  def start_server_process(address, port, *command_line)
    pid = fork do
      exec(*command_line)
    end

    begin
      timeout(1) do
        loop do
          sleep 0.1
          begin
            TCPSocket.new(address, port)
            break
          rescue SystemCallError
          end
        end
      end
    rescue
      stop_server_process(pid)
      raise
    end

    pid
  end

  def start_server
    command_line = [
      groonga,
      "-s",
      "--bind-address", @bind_address,
      "-p", @port.to_s,
      "-e", @encoding,
      "--document-root", document_root,
    ]
    command_line.concat(["--protocol", @protocol]) if @protocol
    command_line.concat(["-n", @database_path])
    @groonga_pid = start_server_process(@bind_address, @port, *command_line)
  end

  def stop_server_process(pid)
    return if pid.nil?
    Process.kill(:TERM, pid)
    begin
      Process.waitpid(pid)
    rescue Errno::ECHILD
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
    normalize_json_result(`#{construct_command_line(*arguments)}`)
  end

  def normalize_json_result(result)
    result.gsub(/^\[\[0,[\d\.e\-]+,[\d\.e\-]+\]/, "[[0,0.0,0.0]")
  end

  def utf8(string)
    string.force_encoding("UTF-8") if string.respond_to?(:force_encoding)
    string
  end

  LANG_ENVS = %w"LANG LC_ALL LC_CTYPE"

  def invoke_command(*args)
    options = args.last.is_a?(Hash) ? args.pop : {}
    input_data = options[:input] || ""
    capture_output = options[:capture_output]
    capture_output = true if capture_output.nil?
    capture_error = options[:capture_error]
    args = [args] unless args.kind_of?(Array)
    begin
      in_child, in_parent = IO.pipe
      out_parent, out_child = IO.pipe if capture_output
      err_parent, err_child = IO.pipe if capture_error
      pid = fork do
        c = "C"
        LANG_ENVS.each {|lc| ENV[lc] = c}
        case args.first
        when Hash
          ENV.update(args.shift)
        end
        STDIN.reopen(in_child)
        in_parent.close
        if capture_output
          STDOUT.reopen(out_child)
          out_parent.close
        end
        if capture_error
          STDERR.reopen(err_child)
          err_parent.close
        end
        Process.setrlimit(Process::RLIMIT_CORE, 0) rescue nil
        exec(*args)
      end
      in_child.close
      out_child.close if capture_output
      err_child.close if capture_error
      th_stdout = Thread.new { out_parent.read } if capture_output
      th_stderr = Thread.new { err_parent.read } if capture_error
      in_parent.write(input_data.to_str)
      in_parent.close
      if (!capture_output || th_stdout.join(60)) &&
          (!capture_error || th_stderr.join(60))
        stdout = th_stdout.value if capture_output
        stderr = th_stderr.value if capture_error
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

  def invoke_groonga(*args)
    environment = args.first.is_a?(Hash) ? args.shift : {}
    args.unshift(groonga)
    args.unshift(environment)
    invoke_command(*args)
  end

  def assert_run_groonga(test_stdout, test_stderr, args, *rest)
    argnum = rest.size + 3
    options = (Hash === rest.last ? rest.pop : {})
    message = (rest.pop if String === rest.last)
    if String === rest.last
      input = rest.pop
    else
      input = options.delete(:input) || ""
    end
    unless rest.empty?
      raise ArgumentError, "wrong number of arguments (#{argnum} for 3)"
    end
    args = [args] unless args.is_a?(Array)
    args << options.merge(:input => input,
                          :capture_output => true,
                          :capture_error => true)
    stdout, stderr, status = invoke_groonga(*args)
    assert_not_predicate(status, :signaled?)
    stdout, stderr = yield(stdout, stderr) if block_given?
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
