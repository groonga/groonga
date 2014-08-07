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
  end
end
