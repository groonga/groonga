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
column_create item gene95_desc COLUMN_SCALAR ShortText
column_create bigram item_gene95_desc COLUMN_INDEX|WITH_POSITION item gene95_desc
load --table item
[["_key","gene95_desc"],
END

while !STDIN.eof?
  key = Kconv.toutf8(gets.strip)
  body = Kconv.toutf8(gets.strip)
  puts [key, body].to_json
end
puts ']'
