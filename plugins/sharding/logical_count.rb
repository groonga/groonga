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
               ])

      def run_body(input)
        enumerator = LogicalEnumerator.new("logical_count", input)

        counter = Counter.new(input, enumerator.target_range)
        total = 0
        have_shard = false
        enumerator.each do |shard, shard_range|
          have_shard = true
          total += counter.count(shard, shard_range)
        end
        unless have_shard
          message =
            "[logical_count] no shard exists: " +
            "logical_table: <#{enumerator.logical_table}>: " +
            "shard_key: <#{enumerator.shard_key_name}>"
          raise InvalidArgument, message
        end
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
        dynamic_columns = DynamicColumns.parse(input)
        key << dynamic_columns.cache_key
        key
      end

      class Counter
        def initialize(input, target_range)
          @logger = Context.instance.logger
          @filter = input[:filter]
          @dynamic_columns = DynamicColumns.parse(input)
          @target_range = target_range
        end

        def count(shard, shard_range)
          cover_type = @target_range.cover_type(shard_range)
          return 0 if cover_type == :none

          shard_key = shard.key
          if shard_key.nil?
            message = "[logical_count] shard_key doesn't exist: " +
                      "<#{shard.key_name}>"
            raise InvalidArgument, message
          end
          table_name = shard.table_name

          prepare_table(shard) do |table|
            if cover_type == :all
              log_use_range_index(false, table_name, "covered",
                                  __LINE__, __method__)
              if @filter
                return filtered_count_n_records(table, shard_key, cover_type)
              else
                return table.size
              end
            end

            range_index = nil
            if @filter
              log_use_range_index(false, table_name, "need filter",
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

            if range_index
              count_n_records_in_range(range_index, cover_type)
            else
              filtered_count_n_records(table, shard_key, cover_type)
            end
          end
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
          @logger.log(Logger::Level::DEBUG,
                      __FILE__,
                      line,
                      method.to_s,
                      message)
        end

        def prepare_table(shard)
          table = shard.table
          return yield(table) if @filter.nil?

          @dynamic_columns.each_initial do |dynamic_column|
            if table == shard.table
              table = table.select_all
            end
            dynamic_column.apply(table)
          end

          begin
            yield(table)
          ensure
            table.close if table != shard.table
          end
        end

        def filtered_count_n_records(table, shard_key, cover_type)
          expression = nil
          filtered_table = nil

          expression_builder = RangeExpressionBuilder.new(shard_key,
                                                          @target_range)
          expression_builder.filter = @filter
          begin
            expression = Expression.create(table)
            case cover_type
            when :all
              expression_builder.build_all(expression)
            when :partial_min
              expression_builder.build_partial_min(expression)
            when :partial_max
              expression_builder.build_partial_max(expression)
            when :partial_min_and_max
              expression_builder.build_partial_min_and_max(expression)
            end
            filtered_table = table.select(expression)
            filtered_table.size
          ensure
            filtered_table.close if filtered_table
            expression.close if expression
          end
        end

        def count_n_records_in_range(range_index, cover_type)
          case cover_type
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

          TableCursor.open(range_index.table,
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
