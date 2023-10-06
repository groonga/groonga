#!/usr/bin/env ruby
#
# Copyright(C) 2023  Sutou Kouhei <kou@clear-code.com>
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
require "find"
require "json"

class Updator
  def initialize(base_dir)
    @base_dir = base_dir
  end

  def update(source)
    FileUtils.rm_rf(@base_dir)
    FileUtils.mkdir_p(@base_dir)

    @processed_files = {}
    @current_db = nil
    @output_log = false
    Find.find(source) do |path|
      case File.extname(path)
      when ".rst"
        update_rst(path)
      end
    end
  end

  private
  def puts(*args)
    $stderr.puts(*args)
  end

  def expand_command_line(command_line)
    command_line.gsub(/\${DB_PATH}/) do
      current_db_path
    end
  end

  def current_db_path
    File.join(@base_dir, @current_db)
  end

  def current_log_path
    "#{current_db_path}.log"
  end

  def run_groonga
    command_line = [
      "groonga",
      "--log-path", current_log_path,
    ]
    command_line << "-n" unless File.exist?(current_db_path)
    command_line << current_db_path
    FileUtils.touch(current_log_path)
    IO.pipe do |in_read, in_write|
      IO.pipe do |out_read, out_write|
        pid = spawn(*command_line, in: in_read, out: out_write)
        in_read.close
        out_write.close
        begin
          File.open(current_log_path) do |log|
            # Ensure initializing
            in_write.puts("status")
            in_write.flush
            out_read.gets

            log.seek(0, IO::SEEK_END)
            @current_log = log
            yield(in_write, out_read)
          end
        ensure
          @current_log = nil
          in_write.close unless in_write.closed?
          out_read.close unless out_read.closed?
          Process.waitpid(pid)
        end
      end
    end
  end

  def read_output(output)
    data = ""
    timeout = 1
    while IO.select([output], nil, nil, timeout)
      break if output.eof?
      data << output.readpartial(4096)
      timeout = 0
    end
    data
  end

  def parse_result(command, result)
    begin
      JSON.parse(result)
    rescue JSON::ParserError
      puts("Failed to parse:")
      puts("path: #{@current_path}")
      puts("command:")
      puts(command)
      puts("result:")
      puts(result)
      raise
    end
  end

  def normalize_error_message(message)
    message.gsub(current_db_path, "${DB_PATH}")
  end

  def normalize_file_name(file_name)
    file_name.gsub(/\A.*?\/lib\//, "lib/")
  end

  def normalize_result(result)
    status = result[0]
    if status
      normalized_start_time = 1337566253.89858
      normalized_elapsed_time = 0.000355720520019531
      status[1] = normalized_start_time
      status[2] = normalized_elapsed_time
      status[3] = normalize_error_message(status[3]) if status[3]
      return_code = status[0]
      if return_code != 0
        backtraces = status[4]
        if backtraces
          backtraces.each do |backtrace|
            file_name = backtrace[1]
            backtrace[1] = normalize_file_name(file_name)
          end
        end
      end
    end
    result
  end

  def normalize_formatted_result(formatted_result)
    formatted_result.gsub(current_db_path, "${DB_PATH}")
  end

  def normalize_log(log)
    log.gsub(/^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+/,
             "2023-10-05 17:26:13.890356")
  end

  def execute_command(input, output, command, current_output_path, output_log)
    input.puts(command)
    input.flush
    is_command = /\A[a-z\/]/.match?(command)
    is_load_command = command.start_with?("load ")
    is_path_style = command.start_with?("/")
    puts(command)
    result = read_output(output)
    if command.start_with?("dump")
      formatted_result = result
    else
      parsed_result = parse_result(command, result)
      normalized_result = normalize_result(parsed_result)
      formatted_result = JSON.generate(normalized_result)
      if formatted_result.bytesize > 79
        formatted_result = JSON.pretty_generate(normalized_result)
      end
      formatted_result = normalize_formatted_result(formatted_result)
    end
    puts(formatted_result)
    if current_output_path
      File.open(current_output_path, "a") do |o|
        command_prefix = "  "
        command_prefix << "$ curl http://localhost:10041" if is_path_style
        o.puts(command.gsub(/^/, command_prefix))
        output_prefix = "  "
        output_prefix << "# " unless is_path_style
        o.puts(formatted_result.gsub(/^/, output_prefix))

        if output_log
          log = normalize_log(read_output(@current_log).strip)
          unless log.empty?
            log_prefix = "# log: "
            puts(log.gsub(/^/, log_prefix))
            o.puts(log.gsub(/^/, "  #{log_prefix}"))
          end
        end
      end
    end
  end

  def update_rst(path)
    if @processed_files.key?(path)
      puts("Skipped processed file: #{path}")
      return
    end
    @processed_files[path] = true
    @current_path = path
    groonga_command_block = ""
    in_groonga_command = false
    File.read(path).each_line do |line|
      if in_groonga_command
        if line.start_with?("..")
          groonga_command_block << line.gsub(/\A\.\. ?/, "")
        else
          in_groonga_command = false
          process_groonga_command(groonga_command_block)
          groonga_command_block.clear
        end
      else
        case line
        when /\A\.\. groonga-command/
          in_groonga_command = true
        when /\A\.\. groonga-include\s*:/
          include_path = line.split(":", 2)[1].strip
          update_rst(File.join(File.dirname(@current_path), include_path))
        end
      end
    end
    unless groonga_command_block.empty?
      process_groonga_command(groonga_command_block)
    end
  end

  def process_groonga_command(groonga_command)
    current_output_path = nil
    actions = []
    command = ""
    in_load_values = false
    groonga_command.each_line do |line|
      case line
      when /\Adatabase:/
        @current_db = line.split(":", 2)[1].strip
      when /\Alog:/
        actions << {
          type: :output_log,
          value: (line.split(":", 2)[1].strip == "true"),
        }
      when /\Ainclude::/
        path = line.split("::", 2)[1].strip
        base_dir = File.dirname(@current_path)
        current_output_path = File.join(base_dir, path)
        puts("### Current output path: #{current_output_path}")
        FileUtils.mkdir_p(File.dirname(current_output_path))
        File.open(current_output_path, "w") do |output|
          output.puts("Execution example::")
          output.puts
        end
      when /\A[%$] /
        actions << {
          type: :command_line,
          value: line.split(/[%$]/, 2)[1].strip,
        }
      when /\A\.\. /
        actions << {
          type: :comment,
          value: line.split("..", 2)[1].strip,
        }
      when /\A#/
        actions << {
          type: :comment,
          value: line,
        }
      else
        next if command.empty? and line == "\n"
        command << line
        next if line.end_with?("\\\n")
        if in_load_values
          in_load_values = (line != "]\n")
        else
          in_load_values = (command.start_with?("load") and
                            not command.include?(" --values "))
        end
        unless in_load_values
          actions << {
            type: :command,
            value: command,
          }
          command = ""
        end
      end
    end
    return if actions.empty?
    run_groonga do |input, output|
      output_log = false
      actions.each do |action|
        case action[:type]
        when :output_log
          output_log = action[:value]
          puts("### Output log: #{output_log}")
        when :command_line
          command_line = action[:value]
          expanded_command_line = expand_command_line(command_line)
          puts(expanded_command_line)
          command_line_output = `#{expanded_command_line}`
          if current_output_path
            File.open(current_output_path, "a") do |output|
              output.puts("  $ #{command_line}")
              output.puts(command_line_output.gsub(/^/, "  "))
            end
          end
          puts(command_line_output)
        when :comment
          comment = action[:value]
          if current_output_path
            File.open(current_output_path, "a") do |output|
              output.puts(comment.gsub(/^/, "  "))
            end
          end
          puts(comment)
        when :command
          command = action[:value]
          execute_command(input, output, command, current_output_path, output_log)
        end
      end
    end
  end
end

if File.directory?("/dev/shm")
  base_dir = "/dev/shm/groonga-doc"
else
  base_dir = "/tmp/groonga-doc"
end
updator = Updator.new(base_dir)
source = ARGV[0] || File.join(__dir__, "source")
updator.update(source)
