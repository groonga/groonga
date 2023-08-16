#!/usr/bin/env ruby

require "English"
require "nkf"
require "json"

print(<<HEADER.chomp)
column_create item_dictionary edict_desc COLUMN_SCALAR ShortText
column_create bigram item_dictionary_edict_desc COLUMN_INDEX|WITH_POSITION item_dictionary edict_desc
load --table item_dictionary
[
["_key","edict_desc","kana"]
HEADER

loop do
  raw_line = gets
  break if raw_line.nil?

  line = raw_line.encode("UTF-8", "EUC-JP")
  keys, body = line.strip.split("/", 2)
  keys = keys.strip
  if /\s*\[(.+)\]\z/ =~ keys
    keys = $PREMATCH
    readings = $1
    body = "[#{readings}] #{body}"
    kana = readings.split(";").collect do |reading|
      NKF.nkf("-Ww --katakana", reading)
    end
    keys.split(";").each do |key|
      puts(",")
      print([key, body, [key, *kana].uniq].to_json)
    end
  else
    keys.split(";").each do |key|
      puts(",")
      kana = NKF.nkf("-Ww --katakana", key)
      print([key, body, [key, kana].uniq].to_json)
    end
  end
end
puts
puts("]")
