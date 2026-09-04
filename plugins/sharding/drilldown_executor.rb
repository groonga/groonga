module Groonga
  module Sharding
    # Executes one drilldown against the given target tables: groups
    # records in all target tables into one result set, applies
    # dynamic columns and filters the result set.
    #
    # This depends only on the given values. It doesn't depend on a
    # command execution context. So this can be used in a child
    # context.
    class DrilldownExecutor
      include QueryLoggable

      # Temporary tables created by #execute. The caller must close
      # them after the result set is no longer used.
      attr_reader :temporary_tables

      # Expressions created by #execute. The caller must close them
      # after the result set is no longer used.
      attr_reader :expressions

      # @param target_tables [Array<Groonga::Table>] Tables to be
      #   grouped. Records in all tables are grouped into one result
      #   set.
      # @param keys [Array<String>] Drilldown keys.
      # @param calc_types [Integer] Groonga::TableGroupFlags::CALC_*
      # @param calc_target_name [String, nil]
      # @param filter [String, nil]
      # @param dynamic_columns [DynamicColumns, nil]
      # @param query_log_prefix [String]
      def initialize(target_tables,
                     keys,
                     calc_types: TableGroupFlags::CALC_COUNT,
                     calc_target_name: nil,
                     filter: nil,
                     dynamic_columns: nil,
                     query_log_prefix: "drilldown")
        @target_tables = target_tables
        @keys = keys
        @calc_types = calc_types
        @calc_target_name = calc_target_name
        @filter = filter
        @dynamic_columns = dynamic_columns
        @query_log_prefix = query_log_prefix
        @temporary_tables = []
        @expressions = []
      end

      # @return [[Groonga::Table, Groonga::Expression]] The result set
      #   and the condition. The condition is `nil` when `filter`
      #   isn't specified.
      def execute
        result_set = group
        @temporary_tables << result_set
        query_logger.log(:size,
                         ":",
                         "#{@query_log_prefix}(#{result_set.size})")
        if @dynamic_columns
          options = {query_log_prefix: "#{@query_log_prefix}."}
          @dynamic_columns.apply_initial([[result_set]], options)
        end
        return [result_set, nil] if @filter.nil?

        expression = Expression.create(result_set)
        @expressions << expression
        expression.parse(@filter)
        filtered_result_set = result_set.select(expression)
        @temporary_tables << filtered_result_set
        query_logger.log(:size,
                         ":",
                         "#{@query_log_prefix}.filter(#{filtered_result_set.size})")
        [filtered_result_set, expression]
      end

      private
      def group
        group_result = TableGroupResult.new
        begin
          group_result.key_begin = 0
          group_result.key_end = @keys.size - 1
          if @keys.size > 1
            group_result.max_n_sub_records = 1
          end
          group_result.limit = 1
          group_result.flags = @calc_types
          @target_tables.each do |table|
            calc_target = nil
            calc_target = table.find_column(@calc_target_name) if @calc_target_name
            group_result.calc_target = calc_target
            begin
              table.group(@keys, group_result)
            ensure
              calc_target.close if calc_target
              group_result.calc_target = nil
            end
          end
          result_set = group_result.table
          group_result.table = nil
          result_set
        ensure
          group_result.close
        end
      end
    end
  end
end
