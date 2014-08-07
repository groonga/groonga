module Groonga
  class Logger
    def log_error(error)
      log_level = Level::ERROR.to_i
      error.backtrace.each do |entry|
        match_data = /:(\d+):/.match(entry)
        file = match_data.pre_match
        line = match_data[1].to_i
        method = match_data.post_match.gsub(/^in /, "")
        log(log_level, file, line, method, match_data.post_match)
      end
    end
  end
end
