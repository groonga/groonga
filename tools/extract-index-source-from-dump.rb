#!/usr/bin/env ruby
#
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

require "English"

require "groonga-log"

parser = GroongaLog::Parser.new

parser.parse(ARGF) do |statistics|
  message = statistics.message
  case statistics.message
  when /\Arecord:/, /\A\(/
    message.scan(/\(\d+:\d+:\d+:\d+\)/) do |raw_record|
      puts(raw_record)
    end
  end
end
