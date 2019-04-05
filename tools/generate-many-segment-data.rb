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

require "json"
require "optparse"

n_records = 50_000_000
max_code_point = 0xFFFF
n_characters_per_record = 100
use_section = false
parser = OptionParser.new
parser.on("--n-records=N", Integer,
          "[#{n_records}]") do |n|
  if n < 0
    n_records = Float::INFINITY
  else
    n_records = n
  end
end
parser.on("--[no-]use-section",
          "[#{use_section}]") do |boolean|
  use_section = boolean
end
parser.parse!

if use_section
  columns = 5.times.collect {|i| "value#{i}"}
else
  columns = ["value"]
end

puts(<<-COMMANDS)
table_create Data TABLE_NO_KEY
COMMANDS
columns.each do |column|
  puts(<<-COMMANDS)
column_create Data #{column} COLUMN_SCALAR|COMPRESS_ZSTD Text
  COMMANDS
end

index_column_flags = "COLUMN_INDEX|WITH_POSITION"
if columns.size > 1
  index_column_flags += "|WITH_SECTION"
end
puts(<<-COMMANDS)
table_create Terms TABLE_PAT_KEY ShortText \
  --normalizer NormalizerNFKC100 \
  --default_tokenizer TokenRegexp
column_create Terms index #{index_column_flags} Data #{columns.join(",")}
COMMANDS

puts(<<-LOAD)
load --table Data
[
LOAD
(1..n_records).each do |id|
  record = {"_id" => id}
  columns.each do |column|
    value = ""
    n_characters_per_record.times do
      while true
        code_point = rand(max_code_point)
        character = [code_point].pack("U")[0]
        if character.valid_encoding?
          value << character
          break
        end
      end
    end
    record[column] = value
  end
  puts(record.to_json)
end
puts(<<-LOAD)
]
LOAD
