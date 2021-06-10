module Groonga
  module ExpressionTree
    class LogicalOperation
      attr_reader :operator
      attr_reader :nodes
      attr_reader :weight_factor
      def initialize(operator, nodes, weight_factor=nil)
        @operator = operator
        @nodes = nodes
        @weight_factor = weight_factor
      end

      def build(expression)
        @nodes.each_with_index do |node, i|
          node.build(expression)
          next if i.zero?
          if @weight_factor
            expression.append_constant(@weight_factor, @operator, 2)
          else
            expression.append_operator(@operator, 2)
          end
        end
      end

      def estimatable?
        @nodes.all? do |node|
          node.estimatable?
        end
      end

      def estimate_size(table)
        estimated_sizes = @nodes.collect do |node|
          node.estimate_size(table)
        end
        case @operator
        when Operator::AND
          estimated_sizes.min
        when Operator::OR
          estimated_sizes.max
        else
          estimated_sizes.first
        end
      end
    end
  end
end
