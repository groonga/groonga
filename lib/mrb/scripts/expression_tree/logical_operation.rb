module Groonga
  module ExpressionTree
    class LogicalOperation
      attr_reader :operator
      attr_reader :nodes
      def initialize(operator, nodes)
        @operator = operator
        @nodes = nodes
      end
    end
  end
end
