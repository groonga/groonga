module Groonga
  module ExpressionTree
    class UnaryOperation
      attr_reader :operator
      attr_reader :value
      def initialize(operator, value)
        @operator = operator
        @value = value
      end

      def build(expression)
        @value.build(expression)
        expression.append_operator(@operator, 1)
      end

      def estimatable?
        true
      end

      def estimate_size(table)
        # TODO
        table.size
      end
    end
  end
end
