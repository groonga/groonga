module Groonga
  module ExpressionTree
    class Assign
      attr_reader :variable
      attr_reader :value
      def initialize(variable, value)
        @variable = variable
        @value = value
      end

      def build(expression)
        @variable.build(expression)
        @value.build(expression)
        expression.append_operator(Operator::ASSIGN, 2)
      end
    end
  end
end
