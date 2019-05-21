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
        @value = resolve_value(value)
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
        self.class.new(@value | other.to_i)
      end

      def &(other)
        other = self.class.new(other) unless other.is_a?(self.class)
        self.class.new(@value & other.to_i)
      end

      private
      def resolve_value(value)
        case value
        when String, Symbol
          resolved_value = VALUES[value.to_sym]
          if resolved_value.nil?
            available_names = VALUES.keys.inspect
            message = "unknown flag name: #{value.inspect}: #{available_names}"
            raise ArgumentError, message
          end
          resolved_value
        when ::Array
          value.inject(0) do |resolved_value, v|
            resolved_value | resolve_value(v)
          end
        else
          value
        end
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
