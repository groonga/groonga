module Groonga
  module Sharding
    # Selects records in a shard by the target range and the given
    # condition.
    #
    # This depends only on the given values. It doesn't depend on a
    # command execution context. So this can be used in a child
    # context.
    class ShardSelector
      # Expressions created by #select. The caller must close them
      # after the result set is no longer used.
      attr_reader :expressions

      # @param table [Groonga::Table] The target table. It may be a
      #   shard table or a temporary table that has records of a
      #   shard table.
      # @param shard_key [Groonga::Column] The shard key column.
      # @param target_range An object that has `min`, `min_border`,
      #   `max` and `max_border`.
      # @param cover_type [Symbol] How the target range covers the
      #   shard: `:all`, `:partial_min`, `:partial_max` or
      #   `:partial_min_and_max`.
      def initialize(table, shard_key, target_range, cover_type, options={})
        @table = table
        @shard_key = shard_key
        @target_range = target_range
        @cover_type = cover_type
        @match_columns = options[:match_columns]
        @query = options[:query]
        @filter = options[:filter]
        @expressions = []
      end

      # @return [[Groonga::Table, Groonga::Expression]] The result set
      #   and the condition. The result set is the target table itself
      #   and the condition is `nil` when the whole target table is
      #   selected without any condition.
      def select
        expression_builder = RangeExpressionBuilder.new(@shard_key,
                                                        @target_range)
        expression_builder.match_columns = @match_columns
        expression_builder.query = @query
        expression_builder.filter = @filter
        begin
          case @cover_type
          when :all
            if @query.nil? and @filter.nil?
              [@table, nil]
            else
              filter_table do |expression|
                expression_builder.build_all(expression)
              end
            end
          when :partial_min
            filter_table do |expression|
              expression_builder.build_partial_min(expression)
            end
          when :partial_max
            filter_table do |expression|
              expression_builder.build_partial_max(expression)
            end
          when :partial_min_and_max
            filter_table do |expression|
              expression_builder.build_partial_min_and_max(expression)
            end
          else
            message = "[shard-selector] unknown cover type: " +
                      "<#{@cover_type.inspect}>"
            raise ArgumentError, message
          end
        ensure
          expression = expression_builder.match_columns_expression
          @expressions << expression if expression
        end
      end

      private
      def filter_table
        expression = Expression.create(@table)
        @expressions << expression
        yield(expression)
        [@table.select(expression), expression]
      end
    end
  end
end
