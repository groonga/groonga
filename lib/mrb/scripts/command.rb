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
    def context
      @context ||= Context.instance
    end

    def run_internal(input)
      begin
        run_body(input)
      rescue => error
        context.record_error(:command_error, error)
        nil
      end
    end

    def output(object)
      context.output(object)
    end
  end
end
