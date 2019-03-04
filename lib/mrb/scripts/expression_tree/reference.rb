module Groonga
  module ExpressionTree
    class Reference
      attr_reader :column
      def initialize(column)
        @column = column
      end

      def build(expression)
        expression.append_object(@column, Operator::GET_REF, 1)
      end

      def estimate_size(table)
        table.size
      end
    end
  end
end
