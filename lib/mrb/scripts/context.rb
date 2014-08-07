module Groonga
  class Context
    def guard(fallback=nil)
      begin
        yield
      rescue => error
        backtrace = error.backtrace
        puts "#{error.class}: #{error.message}"
        backtrace.each do |entry|
          puts entry
        end
        fallback
      end
    end

    def logger
      @logger ||= Logger.new
    end

    def record_error(rc, error)
      rc = RC.find(rc) if rc.is_a?(Symbol)
      self.rc = rc.to_i
      self.error_level = ErrorLevel.find(:error).to_i

      backtrace = error.backtrace
      entry = backtrace.first
      match_data = /:(\d+):/.match(entry)
      self.error_file = match_data.pre_match
      self.error_line = match_data[1].to_i
      self.error_method = match_data.post_match.gsub(/^in /, "")
      self.error_message = error.message

      logger.log_error(error)
    end
  end
end
