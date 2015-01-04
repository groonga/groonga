module Groonga
  class Command
    @@classes = {}
    class << self
      def register_class(name, klass)
        @@classes[name] = klass
      end

      def find_class(name)
        @@classes[name]
      end
    end

    private
    def run_internal(input)
      begin
        run_body(input)
      rescue => error
        Context.instance.record_error(:command_error, error)
        nil
      end
    end
  end
end
