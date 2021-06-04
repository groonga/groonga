#!/bin/ruby

require "open3"
require "json"
require "date"

if ARGV.empty?
  puts "Usage: $0 DATABASE_PATH"
  puts " e.g.: $0 ~/database/db"
  exit(false)
end

DATABASE_PATH = ARGV[0]
ERROR_LOG_NAME = "output_n_buffer_segments.error"

def output_error(status, stdout, stderr)
  puts("#{Time.now}: #{status.inspect}, #{stdout.chomp}, #{stderr.chomp}")
end

stdout, stderr, status = Open3.capture3("groonga",
                                        DATABASE_PATH,
                                        "dump",
                                        "--dump_plugins","no",
                                        "--dump_schema", "no",
                                        "--dump_records", "no",
                                        "--dump_configs", "no")
unless status.success?
  output_error(status, stdout, stderr)
  exit(status.exitstatus)
end

stdout.each_line do |line|
  index_name = line.split[1..2].join(".")
  stdout, stderr, status = Open3.capture3("groonga",
                                          DATABASE_PATH,
                                          "object_inspect",
                                          index_name)
  unless status.success?
    output_error(status, stdout, stderr)
    exit(status.exitstatus)
  end

  result_object_inspect = JSON.parse(stdout)
  unix_time_when_command_executed = result_object_inspect[0][1]
  n_buffer_segments = result_object_inspect[1]["value"]["statistics"]["n_buffer_segments"]

  File.open(index_name, 'a') do |file|
    file.puts("#{unix_time_when_command_executed}, #{n_buffer_segments}")
  end
end
