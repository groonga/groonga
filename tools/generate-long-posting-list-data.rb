#!/usr/bin/env ruby

require "json"

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
100000.times do
  puts({"value" => "X"}.to_json)
end
puts(<<-LOAD)
]
LAOD
