# Copyright(C) 2016-2019 Kouhei Sutou <kou@clear-code.com>
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

module CommandRunner
  class Error < StandardError
    attr_reader :output
    attr_reader :error_output
    def initialize(output, error_output, message)
      @output = output
      @error_output = error_output
      super(message)
    end
  end

  class Result
    attr_reader :output
    attr_reader :error_output
    def initialize(output, error_output)
      @output = output
      @error_output = error_output
    end
  end

  class ExternalProcess
    attr_reader :pid
    attr_reader :input
    attr_reader :output
    def initialize(pid, input, output)
      @pid = pid
      @input = input
      @output = output
    end

    def run_command(command)
      @input.puts(command)
      @input.flush
      @output.gets
    end

    def close
      @input.close unless @input.closed?
      @output.close unless @output.closed?
    end
  end

  class GroongaProcess < ExternalProcess
    def run_command(command)
      parse_response(command, super)
    end

    private
    def parse_response(command, response)
      response = JSON.parse(response)
      normalize_header!(response[0])
      response
    end

    def normalize_header!(header)
      header[1] = 0.0
      header[2] = 0.0
    end
  end

  def run_command(*command_line, **options, &block)
    if block_given?
      run_command_interactive(*command_line, **options, &block)
    else
      run_command_sync(*command_line)
    end
  end

  def groonga(*groonga_command_line, &block)
    if groonga_command_line.last.is_a?(Hash)
      options = groonga_command_line.pop
    end
    command_line = [
      groonga_path,
      "--log-path", @log_path.to_s,
      "--query-log-path", @query_log_path.to_s,
    ]
    if options
      more_command_line = options[:command_line]
      command_line.concat(more_command_line) if more_command_line
    end
    command_line << "-n" unless @database_path.exist?
    command_line << @database_path.to_s
    command_line.concat(groonga_command_line)
    run_command(*command_line,
                external_process_class: GroongaProcess,
                &block)
  end

  def groonga_select(*select_arguments)
    result = groonga("select", *select_arguments)
    select_result = JSON.parse(result.output)
    header, body = select_result
    unless header[0].zero?
      raise "failed to run select: #{select_arguments.join(" ")}: #{PP.pp(select_result, "")}"
    end
    body
  end

  def grndb(command, *arguments)
    command_line = [
      grndb_path,
      command,
      "--log-path", @log_path.to_s,
      @database_path.to_s,
    ]
    command_line.concat(arguments)
    run_command(*command_line)
  end

  def find_program(name, options={})
    name += RbConfig::CONFIG["EXEEXT"]
    ENV["PATH"].split(File::PATH_SEPARATOR).each do |path|
      separator = File::ALT_SEPARATOR || File::SEPARATOR
      program_path = [path, name].join(separator)
      libs_lt_program_path = [path, ".libs", "lt-#{name}"].join(separator)
      libs_program_path = [path, ".libs", name].join(separator)
      if options[:prefer_libtool]
        candidates = [
          libs_lt_program_path,
          libs_program_path,
          program_path,
        ]
      else
        candidates = [
          program_path,
          libs_lt_program_path,
          libs_program_path,
        ]
      end

      candidates.each do |candidate_program_path|
        return candidate_program_path if File.exist?(candidate_program_path)
      end
    end

    name
  end

  def groonga_path
    find_program("groonga")
  end

  def grndb_path
    find_program("grndb")
  end

  def real_grndb_path
    find_program("grndb", :prefer_libtool => true).encode("UTF-8")
  end

  private
  def run_command_interactive(*command_line,
                              external_process_class: ExternalProcess)
    IO.pipe do |input_read, input_write|
      IO.pipe do |output_read, output_write|
        options = {
          :in  => input_read,
          :out => output_write,
          :err => @error_output_log_path.to_s,
        }
        pid = spawn(*command_line, options)
        input_read.close
        output_write.close
        external_process =
          external_process_class.new(pid, input_write, output_read)
        begin
          yield(external_process)
        ensure
          begin
            external_process.close
            Process.waitpid(pid)
          rescue SystemCallError
          end
        end
        error_output = @error_output_log_path.read
        Result.new("", error_output)
      end
    end
  end

  def run_command_sync(*command_line)
    options = {
      :out => @output_log_path.to_s,
      :err => @error_output_log_path.to_s,
    }
    succeeded = system(*command_line, options)
    output = @output_log_path.read.encode("UTF-8", "filesystem")
    error_output = @error_output_log_path.read.encode("UTF-8", "filesystem")
    unless succeeded
      message = <<-MESSAGE.chomp
failed to run: #{command_line.join(" ").encode("UTF-8")}
-- output start --
#{output.chomp}
-- output end --
-- error output start --
#{error_output.chomp}
-- error output end --
      MESSAGE
      raise Error.new(output, error_output, message)
    end
    Result.new(output, error_output)
  end
end
