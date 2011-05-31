# -*- coding: utf-8 -*-
#
# Copyright (C) 2009-2011  Kouhei Sutou <kou@clear-code.com>
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

require "groonga-query-log-analyzer"

module QueryLogAalyzerTest
  class CommandParseTest < Test::Unit::TestCase
    def test_simple
      select = parse("/d/select.json?table=Users&filter=age<=30")
      assert_equal(command("select",
                           "table" => "Users",
                           "filter" => "age<=30",
                           "output_type" => "json"),
                   select)
    end

    private
    def command(name, parameters)
      GroongaQueryLogAnaylzer::Command.new(name, parameters)
    end

    def parse(command_path)
      GroongaQueryLogAnaylzer::Command.parse(command_path)
    end
  end
end
