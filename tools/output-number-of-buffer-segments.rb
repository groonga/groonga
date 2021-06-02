#!/bin/ruby

require "open3"
require "json"

if ARGV.size() == 0
  puts "Usage: $0 DATABASE_PATH"
  puts " e.g.: $0 ~/database/db"
  exit
end

DATABASE_PATH = ARGV[0]
stdout, stderr, status = Open3.capture3("groonga",
                                        DATABASE_PATH,
                                        "dump",
                                        "--dump_plugins","no",
                                        "--dump_schema", "no",
                                        "--dump_records", "no",
                                        "--dump_configs", "no")
stdout.each_line do |line|
  index_name = "#{line.split()[1]}.#{line.split()[2]}"
  stdout, stderr, status = Open3.capture3("groonga",
                                          DATABASE_PATH,
                                          "object_inspect",
                                          index_name)
  File.open(index_name, 'a') do |file|
    file.puts(JSON.parse(stdout)[1]["value"]["statistics"]["n_buffer_segments"])
  end
end
