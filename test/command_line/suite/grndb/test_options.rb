# Copyright(C) 2019 Kouhei Sutou <kou@clear-code.com>
# Copyright(C) 2019 Kentaro Hayashi <hayashi@clear-code.com>
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

class TestGrnDBOptions < GroongaTestCase
  sub_test_case("--log-flags") do
    test("default") do
      groonga("status")
      grndb("check")
      assert_equal("1970-01-01 00:00:00.000000|n| " +
                   "grn_fin (0)",
                   normalize_groonga_log(File.readlines(@log_path).last))
    end

    test("add: one") do
      flags = "+pid"
      groonga("status")
      grndb("check", "--log-flags", flags)
      assert_equal("1970-01-01 00:00:00.000000|n|PROCESS_ID: " +
                   "grn_fin (0)",
                   normalize_groonga_log(File.readlines(@log_path).last))
    end

    test("add: multiple") do
      flags = "+process_id|+thread_id"
      groonga("status")
      grndb("check", "--log-flags", flags)
      assert_equal("1970-01-01 00:00:00.000000|n|PROCESS_ID|THREAD_ID: " +
                   "grn_fin (0)",
                   normalize_groonga_log(File.readlines(@log_path).last))
    end

    test("remove: one") do
      flags = "-time"
      groonga("status")
      grndb("check", "--log-flags", flags)
      assert_equal("|n| grn_fin (0)",
                   normalize_groonga_log(File.readlines(@log_path).last))
    end

    test("remove: multiple") do
      flags = "+pid|-time|-process_id"
      groonga("status")
      grndb("check", "--log-flags", flags)
      assert_equal("|n| grn_fin (0)",
                   normalize_groonga_log(File.readlines(@log_path).last))
    end

    test("replace") do
      flags = "+process_id|default|+thread_id"
      groonga("status")
      grndb("check", "--log-flags", flags)
      assert_equal("1970-01-01 00:00:00.000000|n|THREAD_ID: " +
                   "grn_fin (0)",
                   normalize_groonga_log(File.readlines(@log_path).last))
    end

    test("unknown") do
      flags = "unknown"
      groonga("status")
      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--log-flags", flags)
      end
      assert_equal(<<-MESSAGE, error.error_output)
#{real_grndb_path}: failed to parse log flags: <unknown>
      MESSAGE
    end
  end
end
