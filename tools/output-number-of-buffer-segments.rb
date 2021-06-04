#!/bin/ruby

require "open3"
require "json"
require "date"

if ARGV.empty?
  puts "Usage: $0 DATABASE_PATH"
  puts " e.g.: $0 ~/database/db"
  exit
end

DATABASE_PATH = ARGV[0]
ERROR_LOG_NAME = "output_n_buffer_segments.error"

def output_error(status, stdout, stderr)
  File.open(ERROR_LOG_NAME, 'a') do |file|
    file.puts("#{Time.now}: #{status.inspect}, #{stdout.chomp}, #{stderr.chomp}")
  end
end

stdout, stderr, status = Open3.capture3("groonga",
                                        DATABASE_PATH,
                                        "dump",
                                        "--dump_plugins","no",
                                        "--dump_schema", "no",
                                        "--dump_records", "no",
                                        "--dump_configs", "no")
if (status.success?)
  stdout.each_line do |line|
    index_name = "#{line.split()[1]}.#{line.split()[2]}"
    stdout, stderr, status = Open3.capture3("groonga",
                                            DATABASE_PATH,
                                            "object_inspect",
                                            index_name)
    if (status.success?)
      unix_time_when_command_executed = JSON.parse(stdout)[0][1]
      n_buffer_segments = JSON.parse(stdout)[1]["value"]["statistics"]["n_buffer_segments"]
      File.open(index_name, 'a') do |file|
        file.puts("#{unix_time_when_command_executed}, #{n_buffer_segments}")
      end
    else
      output_error(status, stdout, stderr)
      exit
    end
  end
else
  output_error(status, stdout, stderr)
end
