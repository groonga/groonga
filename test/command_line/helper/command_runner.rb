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

  def run_command(*command_line)
    env = {}
    options = {
      :out => @output_log_path.to_s,
      :err => @error_output_log_path.to_s,
    }
    succeeded = system(env, *command_line, options)
    output = @output_log_path.read
    error_output = @error_output_log_path.read
    unless succeeded
      message = <<-MESSAGE.chomp
failed to run: #{command_line.join(" ")}
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

  def groonga(command, *arguments)
    command_line = [
      "groonga",
      "--log-path", @log_path.to_s,
      "--query-log-path", @query_log_path.to_s,
    ]
    command_line << "-n" unless @database_path.exist?
    command_line << @database_path.to_s
    command_line << command
    command_line.concat(arguments)
    run_command(*command_line)
  end

  def grndb(command, *arguments)
    command_line = [
      "grndb",
      command,
      @database_path.to_s,
    ]
    command_line.concat(arguments)
    run_command(*command_line)
  end

  def find_program(name)
    ENV["PATH"].split(File::PATH_SEPARATOR).each do |path|
      lt_program_path = File.join(path, ".libs", "lt-#{name}")
      return lt_program_path if File.exist?(lt_program_path)

      program_path = File.join(path, name)
      return program_path if File.exist?(program_path)
    end

    nil
  end

  def grndb_path
    find_program("grndb")
  end
end
