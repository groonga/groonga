module Groonga
  module ExpressionTree
    class BinaryOperation
      attr_reader :operator
      attr_reader :left
      attr_reader :right
      def initialize(operator, left, right)
        @operator = operator
        @left = left
        @right = right
      end

      def build(expression)
        @left.build(expression)
        @right.build(expression)
        expression.append_operator(@operator, 2)
      end
    end
  end
end
