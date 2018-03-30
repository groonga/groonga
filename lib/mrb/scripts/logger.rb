module Groonga
  class Logger
    def log(log_level, *args)
      case log_level
      when Symbol
        log_level = Level.find(log_level).to_i
      when String
        log_level = Level.find(log_level.to_sym).to_i
      when Level
        log_level = log_level.to_i
      end

      if args.size == 1
        entry = BacktraceEntry.parse(caller(1, 1)[0])
        file = entry.file
        line = entry.line
        method = entry.method
        message = args[0]
      else
        file, line, method, message = args
      end

      log_raw(log_level, file, line, method, message)
    end

    def log_error(error)
      log_level = Level::ERROR.to_i

      if error.is_a?(Error)
        message = error.message
      else
        message = "#{error.class}: #{error.message}"
      end
      backtrace = error.backtrace
      first_raw_entry = backtrace.first
      if first_raw_entry
        first_entry = BacktraceEntry.parse(first_raw_entry)
        file = first_entry.file
        line = first_entry.line
        method = first_entry.method
        # message = "#{file}:#{line}:#{method}: #{message}"
      else
        file = ""
        line = 0
        method = ""
      end
      log(log_level, file, line, method, message)

      backtrace.each_with_index do |raw_entry, i|
        entry = BacktraceEntry.parse(raw_entry)
        message = entry.message
        message = raw_entry if message.empty?
        log(log_level, entry.file, entry.line, entry.method, raw_entry)
      end
    end
  end
end
