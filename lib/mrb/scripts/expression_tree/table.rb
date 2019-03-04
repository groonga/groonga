module Groonga
  module ExpressionTree
    class Table
      attr_reader :table
      def initialize(table)
        @table = table
      end

      def build(expression)
        expression.append_object(@table, Operator::PUSH, 1)
      end
    end
  end
end
