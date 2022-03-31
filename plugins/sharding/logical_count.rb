module Groonga
  module Sharding
    class LogicalCountCommand < Command
      register("logical_count",
               [
                 "logical_table",
                 "shard_key",
                 "min",
                 "min_border",
                 "max",
                 "max_border",
                 "filter",
                 "post_filter",
               ])

      def run_body(input)
        context = ShardCountContext.new(input)
        begin
          counter = Counter.new(context)
          total = counter.execute
          query_logger.log(:size, ":", "count(#{total})")
          writer.write(total)
        ensure
          context.close
        end
      end

      private
      def cache_key(input)
        key = "logical_count\0"
        key << "#{input[:logical_table]}\0"
        key << "#{input[:shard_key]}\0"
        key << "#{input[:min]}\0"
        key << "#{input[:min_border]}\0"
        key << "#{input[:max]}\0"
        key << "#{input[:max_border]}\0"
        key << "#{input[:filter]}\0"
        key << "#{input[:post_filter]}\0"
        dynamic_columns = DynamicColumns.parse("[logical_count]", input)
        key << dynamic_columns.cache_key
        key
      end

      class ShardCountContext < StreamExecuteContext
        attr_reader :order
        def initialize(input)
          super("logical_count", input)
          @order = :ascending
        end
      end

      class Counter < StreamExecutor
        def initialize(context)
          super(context, ShardCountExecutor)
        end

        def execute
          have_shard = false
          total = 0
          each_shard_executor do |shard_executor|
            have_shard = true
            total += shard_executor.execute
          end
          unless have_shard
            enumerator = @context.enumerator
            message =
              "[logical_count] no shard exists: " +
              "logical_table: <#{enumerator.logical_table}>: " +
              "shard_key: <#{enumerator.shard_key_name}>"
            raise InvalidArgument, message
          end
          total
        end
      end

      class ShardCountExecutor < StreamShardExecutor
        include Loggable
        include QueryLoggable

        def initialize(context, shard, shard_range)
          super("logical_count", context, shard, shard_range)
        end

        def execute
          ensure_filtered

          if @range_index
            return count_n_records_in_range
          end

          return 0 if @filtered_result_sets.empty?

          total = 0
          if @window
            @filtered_result_sets.each do |result_set|
              @window.each(result_set) do |windowed_result_set|
                @temporary_tables << windowed_result_set
                if @context.dynamic_columns.have_filtered?
                  apply_targets = [[windowed_result_set]]
                  @context.dynamic_columns.apply_filtered(apply_targets)
                end
                unless @post_filter.nil?
                  windowed_result_set = apply_post_filter(windowed_result_set)
                  @temporary_tables << windowed_result_set
                end
                total += windowed_result_set.size
              end
            end
          else
            apply_filtered_dynamic_columns
            @filtered_result_sets.each do |result_set|
              unless @post_filter.nil?
                result_set = apply_post_filter(result_set)
                @temporary_tables << result_set
              end
              total += result_set.size
            end
          end
          return total
        end

        def use_range_index?
          if @filter or @post_filter
            decide_use_range_index(false, "need filter",
                                __LINE__, __method__)
          elsif @cover_type == :all
            decide_use_range_index(false, "covered",
                                __LINE__, __method__)
          else
            decide_use_range_index(true, "range index is available",
                                __LINE__, __method__)
          end
        end

        def execute_filter(range_index)
          return if range_index
          if @cover_type == :all and @filter.nil?
            if @post_filter and @context.dynamic_columns.have_filtered?
              filtered_table = @target_table.select_all
              @temporary_tables << filtered_table
              @filtered_result_sets << filtered_table
            else
              @filtered_result_sets << @target_table
            end
          else
            expression = Expression.create(@target_table)
            begin
              expression_builder = RangeExpressionBuilder.new(@shard_key,
                                                              @target_range)
              expression_builder.filter = @filter
              case @cover_type
              when :all
                expression_builder.build_all(expression)
              when :partial_min
                expression_builder.build_partial_min(expression)
              when :partial_max
                expression_builder.build_partial_max(expression)
              when :partial_min_and_max
                expression_builder.build_partial_min_and_max(expression)
              end
              filtered_table = @target_table.select(expression)
            ensure
              expression.close
            end
            @temporary_tables << filtered_table
            @filtered_result_sets << filtered_table
          end
        end

        def count_n_records_in_range
          case @cover_type
          when :partial_min
            min = @target_range.min
            min_border = @target_range.min_border
            max = nil
            max_bordre = nil
          when :partial_max
            min = nil
            min_bordre = nil
            max = @target_range.max
            max_border = @target_range.max_border
          when :partial_min_and_max
            min = @target_range.min
            min_border = @target_range.min_border
            max = @target_range.max
            max_border = @target_range.max_border
          end

          flags = TableCursorFlags::BY_KEY
          case min_border
          when :include
            flags |= TableCursorFlags::GE
          when :exclude
            flags |= TableCursorFlags::GT
          end
          case max_border
          when :include
            flags |= TableCursorFlags::LE
          when :exclude
            flags |= TableCursorFlags::LT
          end

          lexicon = @range_index.table
          @context.referred_objects << lexicon
          TableCursor.open(lexicon,
                           :min => min,
                           :max => max,
                           :flags => flags) do |table_cursor|
            IndexCursor.open(table_cursor, @range_index) do |index_cursor|
              index_cursor.count
            end
          end
        end
      end
    end
  end
end
