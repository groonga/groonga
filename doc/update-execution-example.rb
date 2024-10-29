#!/usr/bin/env ruby
#
# Copyright (C) 2023-2024  Sutou Kouhei <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
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
    @verbose = (ENV["VERBOSE"] == "true")
  end

  def update(source)
    FileUtils.rm_rf(@base_dir)
    FileUtils.mkdir_p(@base_dir)

    @processed_files = {}
    @current_db = nil
    @output_log = false
    Find.find(source) do |path|
      update_file(path)
    end
  end

  private
  def puts(*args)
    return unless @verbose
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
    timeout = 10
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
    file_name = file_name.gsub(/\A\/.*\/\.\.\/plugins\//, "lib/groonga/plugins/")
    file_name = file_name.gsub(/\A.*?\/lib\//, "lib/")
    file_name
  end

  def normalize_result(command, result)
    header = result[0]
    if header
      normalized_start_time = 1337566253.89858
      normalized_elapsed_time = 0.000355720520019531
      header[1] = normalized_start_time
      header[2] = normalized_elapsed_time
      header[3] = normalize_error_message(header[3]) if header[3]
      return_code = header[0]
      if return_code.zero?
        if command.start_with?("status") or command.start_with?("/d/status")
          status = result[1]
          status["alloc_count"] = 29 if status.key?("alloc_count")
          status["starttime"] = 1696558618 if status.key?("starttime")
          status["start_time"] = 1696558618 if status.key?("start_time")
          status["version"] = "2.9.1" if status.key?("version")
          apache_arrow = status["apache_arrow"]
          if apache_arrow
            if apache_arrow.key?("version_major")
              apache_arrow["version_major"] = 2
            end
            if apache_arrow.key?("version_minor")
              apache_arrow["version_minor"] = 9
            end
            if apache_arrow.key?("version_patch")
              apache_arrow["version_patch"] = 1
            end
            if apache_arrow.key?("version")
              apache_arrow["version"] = "2.9.1"
            end
          end
          status["memory_map_size"] = 2929 if status.key?("memory_map_size")
          status["os"] = "Linux" if status.key?("os")
          status["cpu"] = "x86_64" if status.key?("cpu")
        end
      else
        backtraces = header[4]
        if backtraces
          backtraces.each do |backtrace|
            file_name = backtrace[1]
            backtrace[1] = normalize_file_name(file_name)
            # line
            backtrace[2] = 2929 if backtrace[2]
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

  def detect_markup(path)
    File.extname(path)[1..-1].to_sym
  end

  def execute_command(input, output, command, current_output_path, output_log)
    markup = detect_markup(current_output_path)
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
      normalized_result = normalize_result(command, parsed_result)
      formatted_result = JSON.generate(normalized_result)
      if formatted_result.bytesize > 79
        formatted_result = JSON.pretty_generate(normalized_result)
      end
      formatted_result = normalize_formatted_result(formatted_result)
    end
    puts(formatted_result)
    if current_output_path
      File.open(current_output_path, "a") do |o|
        command_prefix = +""
        command_prefix << "  " if markup == :rst
        command_prefix << "$ curl http://localhost:10041" if is_path_style
        o.puts(command.gsub(/^/, command_prefix))
        output_prefix = +""
        output_prefix << "  " if markup == :rst
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

  def update_file(path)
    if @processed_files.key?(path)
      puts("Skipped processed file: #{path}")
      return
    end
    case File.extname(path)
    when ".rst"
      @processed_files[path] = true
      @current_path = path
      update_rst(path)
    when ".md"
      @processed_files[path] = true
      @current_path = path
      update_md(path)
    end
  end

  def update_rst(path)
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
          update_file(File.join(File.dirname(@current_path), include_path))
        end
      end
    end
    unless groonga_command_block.empty?
      process_groonga_command(groonga_command_block)
    end
  end

  def update_md(path)
    groonga_command_block = ""
    in_groonga_command = false
    in_include = false
    File.read(path).each_line do |line|
      if in_groonga_command
        if in_include
          if line.chomp == "```"
            in_groonga_command = false
            in_include = false
            process_groonga_command(groonga_command_block)
            groonga_command_block.clear
          else
            groonga_command_block << line
          end
        else
          case line.chomp
          when /\A```{include} /
            in_include = true
            groonga_command_block << line.gsub(/\A```{include} /, "include::")
          when /\A<!-- database: /
            @current_db = line.split(":", 2)[1].strip
          when ""
            # Ignore
          else
            in_groonga_command = false
          end
        end
      else
        case line.chomp
        when "<!-- groonga-command -->"
          in_groonga_command = true
          in_include = false
        when /\A<!-- groonga-include\s*:/
          include_path = line.split(":", 2)[1].strip
          update_file(File.join(File.dirname(@current_path), include_path))
        end
      end
    end
    unless groonga_command_block.empty?
      process_groonga_command(groonga_command_block)
    end
  end

  def process_groonga_command(groonga_command)
    current_output_path = nil
    markup = nil
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
        markup = detect_markup(current_output_path)
        puts("### Current output path: #{current_output_path}")
        FileUtils.mkdir_p(File.dirname(current_output_path))
        File.open(current_output_path, "w") do |output|
          if markup == :rst
            output.puts("Execution example::")
            output.puts
          else
            output.puts("Execution example:")
            output.puts
          end
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

    in_fenced_code_block = false
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
              if markup == :rst
                output.puts("  $ #{command_line}")
                output.puts(command_line_output.gsub(/^/, "  "))
              else
                if in_fenced_code_block
                  output.puts("```")
                  output.puts
                end
                output.puts("```console")
                output.puts("$ #{command_line}")
                output.puts(command_line_output)
                output.puts("```")
                if in_fenced_code_block
                  output.puts
                  output.puts("```shell")
                end
              end
            end
          end
          puts(command_line_output)
        when :comment
          comment = action[:value]
          if current_output_path
            File.open(current_output_path, "a") do |output|
              if markup == :rst
                output.puts(comment.gsub(/^/, "  "))
              else
                output.puts(comment)
              end
            end
          end
          puts(comment)
        when :command
          command = action[:value]
          if markup == :md and not in_fenced_code_block
            File.open(current_output_path, "a") do |output|
              output.puts("```shell")
            end
            in_fenced_code_block = true
          end
          execute_command(input, output, command, current_output_path, output_log)
        end
      end
    end
    if markup == :md and in_fenced_code_block
      File.open(current_output_path, "a") do |output|
        output.puts("```")
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
source = ARGV[0] || ENV["SOURCES"] || File.join(__dir__, "source")
updator.update(source)
