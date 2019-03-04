module Groonga
  module ExpressionTree
    class AssignBinaryOperation
      attr_reader :operator
      attr_reader :variable
      attr_reader :value
      def initialize(operator, variable, value)
        @operator = operator
        @variable = variable
        @value = value
      end

      def build(expression)
        @variable.build(expression)
        @value.build(expression)
        expression.append_operator(@operator, 2)
      end

      def estimatable?
        false
      end
    end
  end
end
