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
        input_write.close unless input_write.closed?
        Process.waitpid(pid)
      end
    end
  end
end

loaded_db_dir = "#{@db_dir}.loaded"
if File.exist?(loaded_db_dir)
  FileUtils.rm_rf(@db_dir)
  FileUtils.cp_r(loaded_db_dir, @db_dir)
else
  FileUtils.rm_rf(@db_dir)
  FileUtils.mkdir_p(@db_dir)

  n_records = 150000
  n_duplicated_bases = (n_records * 0.15).round
  Faker::Config.random = Random.new(29)

  def generate_date
    Faker::Date.between(from: "2000-01-01", to: "2022-12-31")
  end

  groonga("table_create Tags TABLE_HASH_KEY ShortText")
  groonga("table_create Entries TABLE_HASH_KEY ShortText")
  groonga("column_create Entries tags COLUMN_VECTOR Tags")
  groonga("column_create Tags entries COLUMN_INDEX Entries tags")

  bases = []
  tags_for_base = {}
  groonga do |input|
    input.puts("load --table Entries")
    input.puts("[")
    input.print("[\"_key\", \"tags\"]")
    n_records.times do |i|
      if bases.size >= n_duplicated_bases
        base = bases[Faker::Number.within(range: 0...bases.size)]
      else
        base = Faker::Alphanumeric.alphanumeric(number: 7)
        bases << base
      end
      key = [
        base,
        generate_date.strftime("%Y%M"),
        Faker::Number.within(range: -1..99).to_s,
        Faker::Number.within(range: -1..99).to_s,
        Faker::Number.within(range: -1..99).to_s,
      ].join("_")
      n_tags = Faker::Number.within(range: 0..512)
      n_tags = (n_tags * Faker::Number.normal.abs).round
      tags = tags_for_base[base]
      unless tags
        tags = n_tags.times.collect do
          tag = [
            base,
            generate_date.strftime("%Y%M%d"),
            Faker::Number.within(range: -1..99).to_s,
            Faker::Number.within(range: -1..99).to_s,
          ].join("_")
        end
        tags_for_base[base] = tags
      end
      input.puts(",")
      input.print([key, tags].to_json)
    end
    input.puts
    input.puts("]")
  end

  FileUtils.cp_r(@db_dir, loaded_db_dir)
end

Faker::Config.random = Random.new(29)

groonga("table_create CopiedEntries TABLE_HASH_KEY ShortText")
groonga("column_create CopiedEntries tags COLUMN_VECTOR Tags")

groonga("select Entries " +
        "--limit 0 " +
        "--load_table CopiedEntries " +
        "--load_columns _key,tags " +
        "--load_values _key,tags")

selected = JSON.parse(groonga("select Entries --output_columns _key --limit -1"))
keys = selected[1][0][2..-1].collect(&:first)
keys = keys.sort_by {Faker::Base.rand}
keys.each_slice(1000) do |sliced_keys|
  filter = "in_values(_key"
  sliced_keys.each do |key|
    filter << ", \"#{key}\""
  end
  filter << ")"
  groonga("delete Entries --filter '#{filter}'")
  groonga("select CopiedEntries " +
          "--limit 0 " +
          "--load_table Entries " +
          "--load_columns _key,tags " +
          "--load_values _key,tags")
  inspected = JSON.parse(groonga("object_inspect Tags.entries"))
  puts(inspected[1]["value"]["statistics"]["n_buffer_segments"])
end
