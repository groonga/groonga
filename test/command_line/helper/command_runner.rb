module CommandRunner
  def run_command(*command_line)
    env = {}
    options = {
      :out => @output_log_path.to_s,
      :err => @error_output_log_path.to_s,
    }
    unless system(env, *command_line, options)
      message = <<-MESSAGE.chomp
failed to run: #{command_line.join(" ")}
-- output start --
#{@output_log_path.read.chomp}
-- output end --
-- error output start --
#{@error_output_log_path.read.chomp}
-- error output end --
      MESSAGE
      raise message
    end
    [@output_log_path.read, @error_output_log_path.read]
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
    ]
    command_line << @database_path.to_s
    command_line << command
    command_line.concat(arguments)
    run_command(*command_line)
  end
end
