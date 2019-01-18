#!/usr/bin/env ruby

require "json"
require "optparse"

n_records = 100000
parser = OptionParser.new
parser.on("--n-records=N", Integer,
          "[#{n_records}]") do |n|
  n_records = n
end
parser.parse!

puts(<<-COMMANDS)
table_create Data TABLE_NO_KEY
column_create Data value COLUMN_SCALAR Text

table_create Terms TABLE_PAT_KEY ShortText \
  --normalizer NormalizerNFKC100 \
  --default_tokenizer TokenRegexp
column_create Terms index COLUMN_INDEX|WITH_POSITION Data value
COMMANDS

puts(<<-LOAD)
load --table Data
[
LOAD
n_records.times do
  puts({"value" => "X"}.to_json)
end
puts(<<-LOAD)
]
LOAD
