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
    end
  end
end
