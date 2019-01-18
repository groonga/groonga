#!/usr/bin/env ruby

require "json"
require "optparse"

n_records = 100000
use_section = false
parser = OptionParser.new
parser.on("--n-records=N", Integer,
          "[#{n_records}]") do |n|
  n_records = n
end
parser.on("--[no-]use-section",
          "[#{use_section}]") do |boolean|
  use_section = boolean
end
parser.parse!

if use_section
  columns = 5.times {|i| "value#{i}"}
else
  columns = ["value"]
end

puts(<<-COMMANDS)
table_create Data TABLE_NO_KEY
COMMANDS
columns.each do |column|
  puts(<<-COMMANDS)
column_create Data #{column} COLUMN_SCALAR Text
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
n_records.times do
  record = {}
  columns.each do |column|
    record[column] = "X"
  end
  puts(record.to_json)
end
puts(<<-LOAD)
]
LOAD
