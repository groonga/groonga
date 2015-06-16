module Groonga
  module Sharding
    class LogicalSelectCommand < Command
      register("logical_select",
               [
                 "logical_table",
                 "shard_key",
                 "min",
                 "min_border",
                 "max",
                 "max_border",
                 "filter",
               ])

      def run_body(input)
        output_columns = input[:output_columns] || "_key, *"

        enumerator = LogicalEnumerator.new("logical_select", input)

        context = ExecuteContext.new(input)
        begin
          executor = Executor.new(context)
          executor.execute

          result_sets = context.result_sets
          n_hits = 0
          n_elements = 2 # for N hits and columns
          result_sets.each do |result_set|
            n_hits += result_set.size
            n_elements += result_set.size
          end

          writer.array("RESULTSET", n_elements) do
            writer.array("NHITS", 1) do
              writer.write(n_hits)
            end
            first_result_set = result_sets.first
            if first_result_set
              writer.write_table_columns(first_result_set, output_columns)
            end
            options = {}
            result_sets.each do |result_set|
              writer.write_table_records(result_set, output_columns, options)
            end
          end
        ensure
          context.close
        end
      end

      class ExecuteContext
        attr_reader :enumerator
        attr_reader :filter
        attr_reader :offset
        attr_reader :limit
        attr_reader :result_sets
        def initialize(input)
          @input = input
          @enumerator = LogicalEnumerator.new("logical_select", @input)
          @filter = @input[:filter]
          @offset = 0
          @limit = 10

          @result_sets = []
        end

        def close
          @result_sets.each do |result_set|
            result_set.close if result_set.temporary?
          end
        end
      end

      class Executor
        def initialize(context)
          @context = context
        end

        def execute
          first_table = nil
          enumerator = @context.enumerator
          enumerator.each do |table, shard_key, shard_range|
            first_table ||= table
            next if table.empty?

            shard_executor = ShardExecutor.new(@context,
                                               table, shard_key, shard_range)
            shard_executor.execute
          end
          if first_table.nil?
            message =
              "[logical_select] no shard exists: " +
              "logical_table: <#{enumerator.logical_table}>: " +
              "shard_key: <#{enumerator.shard_key_name}>"
            raise InvalidArgument, message
          end
          if @context.result_sets.empty?
            result_set = HashTable.create(:flags => ObjectFlags::WITH_SUBREC,
                                          :key_type => first_table)
            @context.result_sets << result_set
          end
        end
      end

      class ShardExecutor
        def initialize(context, table, shard_key, shard_range)
          @context = context
          @table = table
          @shard_key = shard_key
          @shard_range = shard_range

          @filter = @context.filter
          @result_sets = @context.result_sets

          @target_range = @context.enumerator.target_range

          @cover_type = @target_range.cover_type(@shard_range)

          @expression_builder = RangeExpressionBuilder.new(@shard_key,
                                                           @target_range,
                                                           @filter)
        end

        def execute
          return if @cover_type == :none

          case @cover_type
          when :all
            filter_shard_all
          when :partial_min
            filter_table do |expression|
              @expression_builder.build_partial_min(expression)
            end
          when :partial_max
            filter_table do |expression|
              @expression_builder.build_partial_max(expression)
            end
          when :partial_min_and_max
            filter_table do |expression|
              @expression_builder.build_partial_min_and_max(expression)
            end
          end
        end

        private
        def filter_shard_all
          if @filter.nil?
            @result_sets << @table
          else
            filter_table do |expression|
              @expression_builder.build_all(expression)
            end
          end
        end

        def create_expression(table)
          expression = Expression.create(table)
          begin
            yield(expression)
          ensure
            expression.close
          end
        end

        def filter_table
          create_expression(@table) do |expression|
            yield(expression)
            @result_sets << @table.select(expression)
          end
        end
      end
    end
  end
end
