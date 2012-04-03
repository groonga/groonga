# -*- coding: utf-8 -*-
#
# Copyright (C) 2010  Nobuyoshi Nakada <nakada@clear-code.com>
# Copyright (C) 2011-2012  Kouhei Sutou <kou@clear-code.com>
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

class OptionTest < Test::Unit::TestCase
  include GroongaTestUtils

  CONFIG_ENV = {"GRN_CONFIG_PATH" => ""}

  def setup
    setup_database_path
  end

  def teardown
    teardown_database_path
  end

  def test_daemon_pid_path
    pid_path = File.join(@tmp_dir, "groonga.pid")
    assert_path_not_exist(pid_path)
    assert_equal("", run_groonga("-d", "--pid-path", pid_path))
    assert_path_exist(pid_path)
    pid = File.open(pid_path) do |f|
      Integer(f.read)
    end
    assert_equal(1, Process.kill(:INT, pid))
    30.times do
      break unless File.exist?(pid_path)
      sleep 0.1
    end
    assert_path_not_exist(pid_path)
  end

  def test_help
    assert_run_groonga(/\AUsage: groonga \[options\.\.\.\] \[dest\]$/,
                       "",
                       ["--help"])
    assert_predicate($?, :success?)
  end

  def test_mandatory_argument_missing
    usage = 'Usage: groonga \[options\.\.\.\] \[dest\]$'
    %w[-e -l -p -i -t
       --document-root --protocol --log-path
       --query-log-path --pid-path --config-path].each do |option|
      status = assert_run_groonga("",
                                  /: option '#{option}' needs argument\.$/,
                                  option)
      assert_not_predicate(status, :success?)
    end
  end

  # FIXME: This test is too dirty. It should be split.
  def test_config_path
    test_options = %W[
      port=1.1.1.1 encoding=none encoding=euc-jp
      max-threads=12345 bind-address=localhost
      log-level=1 server-id=localhost
    ]
    config_file = File.join(@tmp_dir, "test-option.config")
    assert_path_not_exist(config_file)
    status = assert_run_groonga("",
                                /can't open config file: #{Regexp.quote(config_file)} /,
                                [CONFIG_ENV, "--config-path=#{config_file}"])
    assert_not_predicate(status, :success?)
    open(config_file, "w") {}
    status = assert_run_groonga("",
                                "",
                                [CONFIG_ENV, "--config-path=#{config_file}"])
    assert_predicate(status, :success?)

    default_config = run_groonga("--show-config").split(/\n/)

    test_options.each do |opt|
      status = assert_run_groonga(([opt] + default_config).sort.join("\n"),
                                  "",
                                  [CONFIG_ENV,
                                   "--#{opt}",
                                   "--config-path=#{config_file}",
                                   "--show-config"]) do |stdout, stderr|
        [stdout.split(/\n/).sort.join("\n"), stderr]
      end
      assert_predicate(status, :success?)
    end

    test_options.each do |opt|
      open(config_file, "w") {|f| f.puts opt}
      status = assert_run_groonga(([opt] + default_config).sort.join("\n"),
                                  "",
                                  [CONFIG_ENV,
                                   "--config-path=#{config_file}",
                                   "--show-config"]) do |stdout, stderr|
        [stdout.split(/\n/).sort.join("\n"), stderr]
      end
      assert_predicate(status, :success?)
    end
  ensure
    FileUtils.rm_f(config_file)
  end

  def test_default_command_version
    result = JSON.parse(run_groonga("--default-command-version", "1",
                                    "-n", @database_path,
                                    "status"))
    assert_equal(1, result[1]["command_version"])
  end
end
