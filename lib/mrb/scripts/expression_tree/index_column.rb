module Groonga
  module ExpressionTree
    class IndexColumn
      attr_reader :index_column
      def initialize(index_column)
        @index_column = index_column
      end

      def build(expression)
        expression.append_object(@index_column, Operator::PUSH, 1)
      end
    end
  end
end
