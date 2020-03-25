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
        enumerator = LogicalEnumerator.new("logical_count", input)

        counter = Counter.new(input, enumerator.target_range)
        total = 0
        have_shard = false
        begin
          enumerator.each do |shard, shard_range|
            have_shard = true
            counter.count_pre(shard, shard_range)
          end
          total += counter.count
        ensure
          counter.close
        end
        unless have_shard
          message =
            "[logical_count] no shard exists: " +
            "logical_table: <#{enumerator.logical_table}>: " +
            "shard_key: <#{enumerator.shard_key_name}>"
          raise InvalidArgument, message
        end
        query_logger.log(:size, ":", "count(#{total})")
        writer.write(total)
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

      class ShardCountContext
        attr_reader :shard
        attr_reader :cover_type
        attr_reader :range_index
        attr_accessor :table
        def initialize(shard, cover_type, range_index)
          @shard = shard
          @cover_type = cover_type
          @range_idnex = range_index
          @table = shard.table
        end
      end

      class Counter
        include Loggable

        def initialize(input, target_range)
          @filter = input[:filter]
          @post_filter = input[:post_filter]
          @dynamic_columns = DynamicColumns.parse("[logical_count]", input)
          @target_range = target_range
          @contexts = []
          @temporary_tables = []
          @temporary_expressions = []
        end

        def count_pre(shard, shard_range)
          cover_type = @target_range.cover_type(shard_range)
          return if cover_type == :none

          shard_key = shard.key
          if shard_key.nil?
            message = "[logical_count] shard_key doesn't exist: " +
                      "<#{shard.key_name}>"
            raise InvalidArgument, message
          end

          table_name = shard.table_name
          range_index = nil
          if @filter or @post_filter
            log_use_range_index(false, table_name, "need filter",
                                __LINE__, __method__)
          elsif cover_type == :all
            log_use_range_index(false, table_name, "covered",
                                __LINE__, __method__)
          else
            index_info = shard_key.find_index(Operator::LESS)
            range_index = index_info.index if index_info
            if range_index
              log_use_range_index(true, table_name, "range index is available",
                                  __LINE__, __method__)
            else
              log_use_range_index(false, table_name, "no range index",
                                  __LINE__, __method__)
            end
          end
          @contexts << ShardCountContext.new(shard, cover_type, range_index)
        end

        def count
          prepare_contexts
          total = 0
          @contexts.each do |context|
            total += count_shard(context)
          end
          total
        end

        def close
          @temporary_expressions.each(&:close)
          @temporary_tables.each(&:close)
        end

        private
        def log_use_range_index(use, table_name, reason, line, method)
          message = "[logical_count]"
          if use
            message << "[range-index]"
          else
            message << "[select]"
          end
          message << " <#{table_name}>: #{reason}"
          logger.log(Logger::Level::DEBUG,
                     __FILE__,
                     line,
                     method.to_s,
                     message)
        end

        def prepare_contexts
          if @filter or @post_filter
            if @dynamic_columns.have_initial?
              apply_targets = []
              @contexts.each do |context|
                table = context.table.select_all
                @temporary_tables << table
                context.table = table
                apply_targets << [table]
              end
              @dynamic_columns.apply_initial(apply_targets)
            end
          end
          @contexts.each do |context|
            filter_shard(context)
          end
          if @post_filter
            if @dynamic_columns.have_filtered?
              apply_targets = @contexts.collect do |context|
                [context.table]
              end
              @dynamic_columns.apply_filtered(apply_targets)
            end
            @contexts.each do |context|
              post_filter_shard(context)
            end
          end
        end

        def filter_shard(context)
          return if context.range_index

          if context.cover_type == :all and @filter.nil?
            if @post_filter and @dynamic_columns.have_filtered?
              filtered_table = context.table.select_all
              @temporary_tables << filtered_table
              context.table = filtered_table
            end
          else
            expression = Expression.create(context.table)
            @temporary_expressions << expression
            expression_builder = RangeExpressionBuilder.new(context.shard.key,
                                                            @target_range)
            expression_builder.filter = @filter
            case context.cover_type
            when :all
              expression_builder.build_all(expression)
            when :partial_min
              expression_builder.build_partial_min(expression)
            when :partial_max
              expression_builder.build_partial_max(expression)
            when :partial_min_and_max
              expression_builder.build_partial_min_and_max(expression)
            end
            filtered_table = context.table.select(expression)
            @temporary_tables << filtered_table
            context.table = filtered_table
          end
        end

        def count_shard(context)
          if context.range_index
            count_n_records_in_range(context)
          else
            context.table.size
          end
        end

        def post_filter_shard(context)
          expression = nil
          post_filtered_table = nil
          expression = Expression.create(context.table)
          @temporary_expressions << expression
          expression.parse(@post_filter)
          filtered_table = context.table.select(expression)
          @temporary_tables << filtered_table
          context.table = filtered_table
        end

        def count_n_records_in_range(context)
          case context.cover_type
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

          TableCursor.open(context.range_index.table,
                           :min => min,
                           :max => max,
                           :flags => flags) do |table_cursor|
            IndexCursor.open(table_cursor, range_index) do |index_cursor|
              index_cursor.count
            end
          end
        end
      end
    end
  end
end
