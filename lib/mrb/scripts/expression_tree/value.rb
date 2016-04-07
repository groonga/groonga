module Groonga
  module ExpressionTree
    class Value
      attr_reader :code
      def initialize(code)
        @code = code
      end
    end
  end
end
