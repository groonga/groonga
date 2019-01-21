# Copyright(C) 2019 Kouhei Sutou <kou@clear-code.com>
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

class TestGroongaOptions < GroongaTestCase
  sub_test_case("--log-flags") do
    def normalize_init_line(line)
      line.chomp.gsub(/\A
                         (\d{4}-\d{2}-\d{2}\ \d{2}:\d{2}:\d{2}\.\d+)?
                         \|\
                         ([a-zA-Z])
                         \|\
                         ([^: ]+)?
                         ([|:]\ )?
                         (.+)
                      \z/x) do
        timestamp = $1
        level = $2
        id_section = $3
        separator = $4
        message = $5
        timestamp = "1970-01-01 00:00:00.000000" if timestamp
        case id_section
        when nil
        when /\|/
          id_section = "PROCESS_ID|THREAD_ID"
        when /[a-zA-Z]/
          id_section = "THREAD_ID"
        when /\A\d{8,}\z/
          id_section = "THREAD_ID"
        else
          id_section = "PROCESS_ID"
        end
        message = message.gsub(/grn_init: <.+?>/, "grn_init: <VERSION>")
        timestamp.to_s +
          "|" +
          level +
          "|" +
          id_section.to_s +
          separator.to_s +
          message
      end
    end

    test("add: one") do
      groonga("status",
              command_line: ["--log-flags", "+pid"])
      assert_equal("1970-01-01 00:00:00.000000|n|PROCESS_ID: " +
                   "grn_init: <VERSION>",
                   normalize_init_line(File.readlines(@log_path).first))
    end

    test("add: multiple") do
      groonga("status",
              command_line: ["--log-flags", "+process_id|+thread_id"])
      assert_equal("1970-01-01 00:00:00.000000|n|PROCESS_ID|THREAD_ID: " +
                   "grn_init: <VERSION>",
                   normalize_init_line(File.readlines(@log_path).first))
    end

    test("remove: one") do
      groonga("status",
              command_line: ["--log-flags", "-time"])
      assert_equal("|n| grn_init: <VERSION>",
                   normalize_init_line(File.readlines(@log_path).first))
    end

    test("remove: multiple") do
      groonga("status",
              command_line: ["--log-flags", "+pid|-time|-process_id"])
      assert_equal("|n| grn_init: <VERSION>",
                   normalize_init_line(File.readlines(@log_path).first))
    end

    test("replace") do
      groonga("status",
              command_line: ["--log-flags", "+process_id|default|+thread_id"])
      assert_equal("1970-01-01 00:00:00.000000|n|THREAD_ID: " +
                   "grn_init: <VERSION>",
                   normalize_init_line(File.readlines(@log_path).first))
    end
  end
end
