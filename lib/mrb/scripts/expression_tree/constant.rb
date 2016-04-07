module Groonga
  module ExpressionTree
    class Constant
      attr_reader :value
      def initialize(value)
        @value = value
      end

      def build(expression)
        expression.append_constant(@value, Operator::PUSH, 1)
      end
    end
  end
end
