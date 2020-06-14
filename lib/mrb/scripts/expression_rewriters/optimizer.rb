module Groonga
  module ExpressionRewriters
    class Optimizer < ExpressionRewriter
      register "optimizer"

      def rewrite
        builder = ExpressionTreeBuilder.new(@expression)
        root_node = builder.build
        return @expression if root_node.nil?

        variable = @expression[0]
        table = context[variable.domain_id]
        begin
          optimized_root_node = optimize_node(table, root_node)
          rewritten = Expression.create(table)
          optimized_root_node.build(rewritten)
          rewritten
        ensure
          table.unref
        end
      end

      private
      def optimize_node(table, node)
        case node
        when ExpressionTree::LogicalOperation
          optimize_and_not(table, node) do |optimized_node|
            optimized_sub_nodes = optimized_node.nodes.collect do |sub_node|
              optimize_node(table, sub_node)
            end
            case optimized_node.operator
            when Operator::AND
              optimized_sub_nodes =
                optimize_and_sub_nodes(table, optimized_sub_nodes)
            when Operator::OR
              optimized_sub_nodes =
                optimize_or_sub_nodes(table, optimized_sub_nodes)
            end
            if optimized_sub_nodes.size == 1
              optimized_sub_nodes[0]
            else
              ExpressionTree::LogicalOperation.new(optimized_node.operator,
                                                   optimized_sub_nodes)
            end
          end
        when ExpressionTree::BinaryOperation
          optimize_binary_operation_node(table, node)
        else
          node
        end
      end

      def optimize_and_not(table, node)
        return yield(node) unless node.operator == Operator::AND

        optimized_sub_nodes = []
        sub_nodes = []
        node.nodes.each do |sub_node|
          if sub_node.respond_to?(:operator)
            sub_node_operator = sub_node.operator
          else
            sub_node_oeprator = nil
          end
          unless sub_node_operator == Operator::NOT
            sub_nodes << sub_node
            next
          end

          case sub_nodes.size
          when 0
            sub_nodes << sub_node
          when 1
            and_not_node =
              ExpressionTree::LogicalOperation.new(Operator::AND_NOT,
                                                   [sub_nodes.first,
                                                    sub_node.value])
            optimized_sub_nodes << yield(and_not_node)
            sub_nodes = []
          else
            and_nodes = ExpressionTree::LogicalOperation.new(node.operator,
                                                             sub_nodes)
            optimized_and_nodes = yield(and_nodes)
            and_not_node =
              ExpressionTree::LogicalOperation.new(Operator::AND_NOT,
                                                   [optimized_and_nodes,
                                                    sub_node.value])
            optimized_sub_nodes << yield(and_not_node)
            sub_nodes = []
          end
        end
        unless sub_nodes.empty?
          and_nodes = ExpressionTree::LogicalOperation.new(node.operator,
                                                           sub_nodes)
          optimized_sub_nodes << yield(and_nodes)
        end
        if optimized_sub_nodes.size == 1
          optimized_sub_nodes.first
        else
          ExpressionTree::LogicalOperation.new(node.operator,
                                               optimized_sub_nodes)
        end
      end

      def optimize_binary_operation_node(table, node)
        optimized_left = optimize_node(table, node.left)
        optimized_right = optimize_node(table, node.right)
        optimized_options = {}
        if node.parameter
          optimized_options[:parameter] = optimize_node(table, node.parameter)
        end
        optimized_options[:value] = node.value
        case node.operator
        when Operator::EQUAL
          if optimized_left.is_a?(ExpressionTree::Constant) and
            optimized_right.is_a?(ExpressionTree::Variable)
            ExpressionTree::BinaryOperation.new(node.operator,
                                                optimized_right,
                                                optimized_left,
                                                optimized_options)
          else
            ExpressionTree::BinaryOperation.new(node.operator,
                                                optimized_left,
                                                optimized_right,
                                                optimized_options)
          end
        else
          ExpressionTree::BinaryOperation.new(node.operator,
                                              optimized_left,
                                              optimized_right,
                                              optimized_options)
        end
      end

      def group_nodes(nodes)
        nodes.group_by do |node|
          case node
          when ExpressionTree::BinaryOperation
            if node.left.is_a?(ExpressionTree::Variable)
              node.left.column
            else
              nil
            end
          else
            nil
          end
        end
      end

      def optimize_and_sub_nodes(table, sub_nodes)
        return sub_nodes unless sub_nodes.all?(&:estimatable?)

        grouped_sub_nodes = group_nodes(sub_nodes)

        optimized_nodes = []
        grouped_sub_nodes.each do |column, grouped_nodes|
          if column
            grouped_nodes = optimize_grouped_and_nodes(column, grouped_nodes)
          end
          optimized_nodes.concat(grouped_nodes)
        end

        optimized_nodes.sort_by do |node|
          node.estimate_size(table)
        end
      end

      COMPARISON_OPERATORS = [
        Operator::EQUAL,
        Operator::NOT_EQUAL,
        Operator::LESS,
        Operator::GREATER,
        Operator::LESS_EQUAL,
        Operator::GREATER_EQUAL,
      ]
      def optimize_grouped_and_nodes(column, grouped_nodes)
        target_nodes, done_nodes = grouped_nodes.partition do |node|
          node.is_a?(ExpressionTree::BinaryOperation) and
            COMPARISON_OPERATORS.include?(node.operator) and
            node.right.is_a?(ExpressionTree::Constant)
        end

        # TODO: target_nodes = remove_needless_nodes(target_nodes)
        # e.g.: x < 1 && x < 3 -> x < 1: (x < 3) is meaningless

        if target_nodes.size == 2
          between_node = try_optimize_between(column, target_nodes)
          if between_node
            done_nodes << between_node
          else
            done_nodes.concat(target_nodes)
          end
        else
          done_nodes.concat(target_nodes)
        end

        done_nodes
      end

      def try_optimize_between(column, target_nodes)
        greater_node = nil
        less_node = nil
        target_nodes.each do |node|
          case node.operator
          when Operator::GREATER, Operator::GREATER_EQUAL
            greater_node = node
          when Operator::LESS, Operator::LESS_EQUAL
            less_node = node
          end
        end
        return nil if greater_node.nil? or less_node.nil?

        between = ExpressionTree::Procedure.new(context["between"])
        if greater_node.operator == Operator::GREATER
          greater_border = "exclude"
        else
          greater_border = "include"
        end
        if less_node.operator == Operator::LESS
          less_border = "exclude"
        else
          less_border = "include"
        end
        arguments = [
          ExpressionTree::Variable.new(column),
          greater_node.right,
          ExpressionTree::Constant.new(greater_border),
          less_node.right,
          ExpressionTree::Constant.new(less_border),
        ]
        ExpressionTree::FunctionCall.new(between, arguments)
      end

      def optimize_or_sub_nodes(table, sub_nodes)
        grouped_sub_nodes = group_nodes(sub_nodes)

        optimized_nodes = []
        grouped_sub_nodes.each do |column, grouped_nodes|
          if column
            grouped_nodes = optimize_grouped_or_nodes(column, grouped_nodes)
          end
          optimized_nodes.concat(grouped_nodes)
        end
        optimized_nodes
      end

      def optimize_grouped_or_nodes(column, grouped_nodes)
        target_nodes, done_nodes = grouped_nodes.partition do |node|
          node.is_a?(ExpressionTree::BinaryOperation) and
            node.operator == Operator::EQUAL and
            node.right.is_a?(ExpressionTree::Constant)
        end

        if target_nodes.size > 1
          in_values_node = try_optimize_in_values(column, target_nodes)
          if in_values_node
            done_nodes << in_values_node
          else
            done_nodes.concat(target_nodes)
          end
        else
          done_nodes.concat(target_nodes)
        end

        done_nodes
      end

      def try_optimize_in_values(column, target_nodes)
        in_values = ExpressionTree::Procedure.new(context["in_values"])
        arguments = [ExpressionTree::Variable.new(column)]
        target_nodes.each do |node|
          arguments << node.right
        end
        ExpressionTree::FunctionCall.new(in_values, arguments)
      end
    end
  end
end
