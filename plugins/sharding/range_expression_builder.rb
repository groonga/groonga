module Groonga
  module Sharding
    class RangeExpressionBuilder < ExpressionBuilder
      def initialize(key, target_range)
        super()
        @key = key
        @target_range = target_range
      end

      def build(expression, shard_range)
        cover_type = @target_range.cover_type(shard_range)
        case cover_type
        when :all
          build_all(expression)
        when :partial_min
          build_partial_min(expression)
        when :partial_max
          build_partial_max(expression)
        when :partial_min_and_max
          build_partial_min_and_max(expression)
        when :none
        end
      end

      def build_all(expression)
        build_condition(expression)
      end

      def build_partial_min(expression)
        expression.append_object(@key, Operator::PUSH, 1)
        expression.append_operator(Operator::GET_VALUE, 1)
        expression.append_constant(@target_range.min, Operator::PUSH, 1)
        if @target_range.min_border == :include
          expression.append_operator(Operator::GREATER_EQUAL, 2)
        else
          expression.append_operator(Operator::GREATER, 2)
        end
        build_condition(expression)
      end

      def build_partial_max(expression)
        expression.append_object(@key, Operator::PUSH, 1)
        expression.append_operator(Operator::GET_VALUE, 1)
        expression.append_constant(@target_range.max, Operator::PUSH, 1)
        if @target_range.max_border == :include
          expression.append_operator(Operator::LESS_EQUAL, 2)
        else
          expression.append_operator(Operator::LESS, 2)
        end
        build_condition(expression)
      end

      def build_partial_min_and_max(expression)
        between = Groonga::Context.instance["between"]
        expression.append_object(between, Operator::PUSH, 1)
        expression.append_object(@key, Operator::PUSH, 1)
        expression.append_operator(Operator::GET_VALUE, 1)
        expression.append_constant(@target_range.min, Operator::PUSH, 1)
        expression.append_constant(@target_range.min_border,
                                   Operator::PUSH, 1)
        expression.append_constant(@target_range.max, Operator::PUSH, 1)
        expression.append_constant(@target_range.max_border,
                                   Operator::PUSH, 1)
        expression.append_operator(Operator::CALL, 5)
        build_condition(expression)
      end
    end
  end
end
