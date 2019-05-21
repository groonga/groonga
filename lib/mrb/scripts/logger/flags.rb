module Groonga
  class Logger
    class Flags
      VALUES = {
        time: 1,
        title: 2,
        message: 4,
        location: 8,
        process_id: 16,
        thread_id: 32,
      }

      attr_reader :value
      def initialize(value)
        case value
        when String, Symbol
          @value = VALUES[value.to_sym]
          if @value.nil?
            available_names = VALUES.keys.inspect
            message = "unknown flag name: #{value.inspect}: #{available_names}"
            raise ArgumentError, message
          end
        else
          @value = value
        end
      end

      def to_i
        @value
      end

      def to_s
        names = []
        VALUES.each do |name, value|
          names << name.to_s unless (value & @value) == 0
        end
        names.join("|")
      end

      def |(other)
        other = self.class.new(other) unless other.is_a?(self.class)
        new(@value | other.to_i)
      end

      def &(other)
        other = self.class.new(other) unless other.is_a?(self.class)
        new(@value & other.to_i)
      end

      NONE = new(0)
      TIME = new(:time)
      TITLE = new(:title)
      MESSAGE = new(:message)
      LOCATION = new(:location)
      PROCESS_ID = new(:process_id)
      PID = PROCESS_ID
      THREAD_ID = new(:thread_id)
    end
  end
end
