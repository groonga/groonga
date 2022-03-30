#!/usr/bin/env ruby
#
# Copyright(C) 2022  Sutou Kouhei <kou@clear-code.com>
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

require "fileutils"
require "json"

require "faker"

@db_dir = "/tmp/db"

FileUtils.rm_rf(@db_dir)
FileUtils.mkdir_p(@db_dir)

def groonga(input=nil)
  command_line = [
    "groonga",
    "--log-path", "#{@db_dir}/groonga.log",
    "--query-log-path", "#{@db_dir}/query.log",
  ]
  db = "#{@db_dir}/db"
  unless File.exist?(db)
    command_line << "-n"
  end
  command_line << db
  IO.pipe do |input_read, input_write|
    IO.pipe do |output_read, output_write|
      pid = spawn(*command_line, in: input_read, out: output_write)
      begin
        input_read.close
        output_write.close
        if block_given?
          yield(input_write)
        else
          input_write.puts(input)
        end
        input_write.close
        output_read.read
      ensure
        Process.waitpid(pid)
      end
    end
  end
end

groonga("table_create Tags TABLE_HASH_KEY ShortText")
groonga("table_create Entries TABLE_HASH_KEY ShortText")
groonga("column_create Entries tags COLUMN_VECTOR Tags")
groonga("column_create Tags entries COLUMN_INDEX Entries tags")

n_records = 150000
Faker::Config.random = Random.new(29)

def generate_date
  Faker::Date.between(from: "2000-01-01", to: "2022-12-31")
end

keys = []
groonga do |input|
  input.puts("load --table Entries")
  input.puts("[")
  input.print("[\"_key\", \"tags\"]")
  n_records.times do |i|
    base = Faker::Alphanumeric.alphanumeric(number: 7)
    key = [
      base,
      generate_date.strftime("%Y%M"),
      Faker::Number.within(range: -1..99).to_s,
      Faker::Number.within(range: -1..99).to_s,
      Faker::Number.within(range: -1..99).to_s,
    ].join("_")
    keys << key
    n_tags = Faker::Number.within(range: 0..512)
    n_tags = (n_tags * Faker::Number.normal.abs).round
    tags = n_tags.times.collect do
      tag = [
        base,
        generate_date.strftime("%Y%M%d"),
        Faker::Number.within(range: -1..99).to_s,
        Faker::Number.within(range: -1..99).to_s,
      ].join("_")
    end
    input.puts(",")
    input.print([key, tags].to_json)
  end
  input.puts
  input.puts("]")
end

keys = keys.sort_by {Faker::Base.rand}
keys.each_with_index do |key, i|
  groonga("delete Entries #{key}")
  if (i % 1000).zero?
    inspected = JSON.parse(groonga("object_inspect Tags.entries"))
    puts(inspected[1]["value"]["statistics"]["n_buffer_segments"])
  end
end
