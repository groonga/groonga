#!/usr/bin/env ruby
#
# Copyright (C) 2024  Abe Tomoaki <abe@clear-code.com>
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

with_delete = ARGV.include?("--with-delete")

N_RECORDS = 2 ** 29
N_DELETE_RECORDS = 1000
DELETE_START = N_DELETE_RECORDS * 4

puts(<<-COMMANDS)
table_create Data TABLE_HASH_KEY Int64
COMMANDS

puts(<<-LOAD)
load --table Data
[
LOAD
N_RECORDS.times do |i|
  if with_delete and i == DELETE_START
    puts
    puts(<<-LOAD)
]
LOAD
    puts("delete Data --filter _key<#{N_DELETE_RECORDS}")
    puts(<<-LOAD)
load --table Data
[
LOAD
  else
    puts(",") unless i.zero?
  end
  print("{\"_key\": #{i}}")
end
puts
puts(<<-LOAD)
]
LOAD

if with_delete
  puts(<<-LOAD)
load --table Data
[
LOAD
  N_DELETE_RECORDS.times do |i|
    puts(",") unless i.zero?
    print("{\"_key\": #{i}}")
  end
  puts
  puts(<<-LOAD)
]
LOAD
end

$stderr.puts("# select Data: must returns #{N_RECORDS}")
puts("select Data")
if with_delete
  minimal_key = N_DELETE_RECORDS
else
  minimal_key = 0
end
$stderr.puts("# select Data --filter _key==#{minimal_key}: must returns 1 record")
puts("select Data --filter _key==#{N_DELETE_RECORDS}")
$stderr.puts("# select Data --filter _key==#{N_RECORDS - 1}: must returns 1 record")
puts("select Data --filter _key==#{N_RECORDS - 1}")

$stderr.puts("# load: new key must be failed because the hash table is full")
puts(<<-LOAD)
load --table Data
[
{"_key": #{N_RECORDS}}
]
LOAD

$stderr.puts("# load: existing key must be succeeded even when the hash table is full")
puts(<<-LOAD)
load --table Data
[
{"_key": #{minimal_key}}
]
LOAD

$stderr.puts("# load: can re-add a new key after a delete")
puts(<<-LOAD)
delete --table Data --key #{minimal_key}
load --table Data
[
{"_key": #{N_RECORDS}}
]
LOAD
