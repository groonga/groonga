#!/usr/bin/ruby
# -*- coding: utf-8 -*-

$KCODE = 'u'

require 'kconv'

class String
  def to_json
    a = split(//).map {|char|
      case char
      when '"' then '\\"'
      when '\\' then '\\\\'
      when "\b" then '\b'
      when "\f" then '\f'
      when "\n" then '\n'
      when "\r" then ''
      when "\t" then '\t'
      else char
      end
    }
    "\"#{a.join('')}\""
  end
end

class Array
  def to_json
    '[' + map {|element|
      element.to_json
    }.join(',') + ']'
  end
end

puts <<END
column_create item_dictionary edict_desc COLUMN_SCALAR ShortText
column_create bigram item_dictionary_edict_desc COLUMN_INDEX|WITH_POSITION item_dictionary edict_desc
load --table item_dictionary
[["_key","edict_desc"],
END

while !STDIN.eof?
  line = Kconv.toutf8(gets.strip)
  key, body = line.split('/', 2)
  puts [key, body].to_json
end
puts ']'
