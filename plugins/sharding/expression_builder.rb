module Groonga
  module Sharding
    class ExpressionBuilder
      attr_reader :match_columns_expression

      attr_writer :match_columns
      attr_writer :query
      attr_writer :query_flags
      attr_writer :filter

      def initialize
        @match_columns_expression = nil
        @match_columns = nil
        @query = nil
        @query_flags = nil
        @filter = nil
      end

      def build_condition(expression)
        @match_columns_expression = nil
        if @query
          is_empty = expression.empty?
          if @match_columns
            table = Context.instance[expression[0].domain_id]
            @match_columns_expression = Expression.create(table)
            @match_columns_expression.parse(@match_columns)
          end
          flags = Expression::SYNTAX_QUERY
          if @query_flags
            flags |= @query_flags
          else
            flags |= Expression::ALLOW_PRAGMA | Expression::ALLOW_COLUMN
          end
          expression.parse(@query,
                           default_column: @match_columns_expression,
                           flags: flags)
          expression.append_operator(Operator::AND, 2) unless is_empty
        end

        if @filter
          is_empty = expression.empty?
          expression.parse(@filter)
          expression.append_operator(Operator::AND, 2) unless is_empty
        end
      end
    end
  end
end
