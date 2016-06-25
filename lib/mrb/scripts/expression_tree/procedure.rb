module Groonga
  module ExpressionTree
    class Procedure
      attr_reader :value
      def initialize(value)
        @value = value
      end

      def name
        @value.name
      end

      def build(expression)
        expression.append_object(@value, Operator::PUSH, 1)
      end
    end
  end
end
