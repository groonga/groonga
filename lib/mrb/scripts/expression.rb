module Groonga
  class Expression
    def build_scan_info(op, size)
      begin
        builder = ScanInfoBuilder.new(self, op, size)
        builder.build
      rescue => error
        backtrace = error.backtrace
        puts "#{error.class}: #{error.message}"
        backtrace.each do |entry|
          puts entry
        end
        nil
      end
    end
  end
end
