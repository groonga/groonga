module Groonga
  module ExpressionTree
    class LogicalOperation
      attr_reader :operator
      attr_reader :nodes
      def initialize(operator, nodes)
        @operator = operator
        @nodes = nodes
      end

      def build(expression)
        @nodes.each_with_index do |node, i|
          node.build(expression)
          expression.append_operator(@operator, 2) if i > 1
        end
      end
    end
  end
end
