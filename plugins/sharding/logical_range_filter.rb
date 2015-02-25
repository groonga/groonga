module Groonga
  module Sharding
    class LogicalRangeFilterCommand < Command
      register("logical_range_filter",
               [
                 "logical_table",
                 "shard_key",
                 "min",
                 "min_border",
                 "max",
                 "max_border",
                 "order",
                 "filter",
                 "offset",
                 "limit",
                 "output_columns",
               ])

      def run_body(input)
        output_columns = input[:output_columns] || "_key, *"

        executor = Executor.new(input)
        begin
          executor.execute

          result_sets = executor.result_sets
          if result_sets.empty?
            n_elements = 0
          else
            n_elements = 1 # for columns
            result_sets.each do |result_set|
              n_elements += result_set.size
            end
          end

          writer.array("RESULTSET", n_elements) do
            first_result_set = result_sets.first
            if first_result_set
              writer.write_table_columns(first_result_set, output_columns)
            end
            result_sets.each do |result_set|
              writer.write_table_records(result_set, output_columns)
            end
          end
        ensure
          executor.close
        end
      end

      class Executor
        attr_reader :result_sets
        def initialize(input)
          @input = input
          @enumerator = LogicalEnumerator.new("logical_range_filter", @input)
          @order = parse_order(@input, :order)
          @filter = @input[:filter]
          @offset = (@input[:offset] || 0).to_i
          @limit = (@input[:limit] || 10).to_i

          @result_sets = []
          @unsorted_result_sets = []
          @current_offset = 0
          @current_limit = @limit
        end

        def shard_key_name
          @enumerator.shard_key_name
        end

        def execute
          @enumerator.each do |table, shard_key, shard_range|
            filter_shard(table, @filter,
                         shard_key, shard_range,
                         @enumerator.target_range)
            break if @current_limit == 0
          end
        end

        def close
          @unsorted_result_sets.each do |result_set|
            result_set.close if result_set.temporary?
          end
          @result_sets.each do |result_set|
            result_set.close if result_set.temporary?
          end
        end

        private
        def parse_order(input, name)
          order = input[name]
          return :ascending if order.nil?

          case order
          when "ascending"
            :ascending
          when "descending"
            :descending
          else
            message =
              "[logical_range_filter] #{name} must be " +
              "\"ascending\" or \"descending\": <#{order}>"
            raise InvalidArgument, message
          end
        end

        def filter_shard(table, filter, shard_key, shard_range, target_range)
          return if table.empty?

          cover_type = target_range.cover_type(shard_range)
          return if cover_type == :none

          index_info = shard_key.find_index(Operator::LESS)
          if index_info
            range_index = index_info.index
          else
            range_index = nil
          end

          if cover_type == :all
            if filter.nil?
              if table.size <= @current_offset
                @current_offset -= table.size
                return
              end
              if range_index
                filter_by_range(range_index, nil,
                                nil, nil,
                                nil, nil)
              else
                sort_result_set(table)
              end
            else
              filter_table(table, filter)
            end
            return
          end

          if range_index
            # TODO: Determine whether range index is used by estimated size.
            use_range_index = true
          else
            use_range_index = false
          end

          case cover_type
          when :partial_min
            if use_range_index
              filter_by_range(range_index, filter,
                              target_range.min, target_range.min_border,
                              nil, nil)
            else
              filter_table(table, filter) do |expression|
                expression.append_object(shard_key, Operator::PUSH, 1)
                expression.append_operator(Operator::GET_VALUE, 1)
                expression.append_constant(target_range.min, Operator::PUSH, 1)
                if target_range.min_border == :include
                  expression.append_operator(Operator::GREATER_EQUAL, 2)
                else
                  expression.append_operator(Operator::GREATER, 2)
                end
              end
            end
          when :partial_max
            if use_range_index
              filter_by_range(range_index, filter,
                              nil, nil,
                              target_range.max, target_range.max_border)
            else
              filter_table(table, filter) do |expression|
                expression.append_object(shard_key, Operator::PUSH, 1)
                expression.append_operator(Operator::GET_VALUE, 1)
                expression.append_constant(target_range.max, Operator::PUSH, 1)
                if target_range.max_border == :include
                  expression.append_operator(Operator::LESS_EQUAL, 2)
                else
                  expression.append_operator(Operator::LESS, 2)
                end
              end
            end
          when :partial_min_and_max
            if use_range_index
              filter_by_range(range_index, filter,
                              target_range.min, target_range.min_border,
                              target_range.max, target_range.max_border)
            else
              filter_table(table, filter) do |expression|
                expression.append_object(context["between"], Operator::PUSH, 1)
                expression.append_object(shard_key, Operator::PUSH, 1)
                expression.append_operator(Operator::GET_VALUE, 1)
                expression.append_constant(target_range.min, Operator::PUSH, 1)
                expression.append_constant(target_range.min_border,
                                           Operator::PUSH, 1)
                expression.append_constant(target_range.max, Operator::PUSH, 1)
                expression.append_constant(target_range.max_border,
                                           Operator::PUSH, 1)
                expression.append_operator(Operator::CALL, 5)
              end
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

        def filter_by_range(range_index, filter,
                            min, min_border, max, max_border)
          lexicon = range_index.domain
          data_table = range_index.range
          flags = build_range_search_flags(min_border, max_border)

          result_set = HashTable.create(:flags => ObjectFlags::WITH_SUBREC,
                                        :key_type => data_table)
          n_matched_records = 0
          begin
            TableCursor.open(lexicon,
                             :min => min,
                             :max => max,
                             :flags => flags) do |table_cursor|
              options = {
                :offset => @current_offset,
                :limit => @current_limit,
              }
              if filter
                create_expression(data_table) do |expression|
                  expression.parse(filter)
                  options[:expression] = expression
                  IndexCursor.open(table_cursor, range_index) do |index_cursor|
                    n_matched_records = index_cursor.select(result_set, options)
                  end
                end
              else
                IndexCursor.open(table_cursor, range_index) do |index_cursor|
                  n_matched_records = index_cursor.select(result_set, options)
                end
              end
            end
          rescue
            result_set.close
            raise
          end

          if n_matched_records <= @current_offset
            @current_offset -= n_matched_records
            result_set.close
            return
          end

          if @current_offset > 0
            @current_offset = 0
          end
          @current_limit -= result_set.size
          @result_sets << result_set
        end

        def build_range_search_flags(min_border, max_border)
          flags = TableCursorFlags::BY_KEY
          case @order
          when :ascending
            flags |= TableCursorFlags::ASCENDING
          when :descending
            flags |= TableCursorFlags::DESCENDING
          end
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
          flags
        end

        def filter_table(table, filter)
          create_expression(table) do |expression|
            if block_given?
              yield(expression)
              if filter
                expression.parse(filter)
                expression.append_operator(Operator::AND, 2)
              end
            else
              expression.parse(filter)
            end
            result_set = table.select(expression)
            sort_result_set(result_set)
          end
        end

        def sort_result_set(result_set)
          if result_set.empty?
            result_set.close if result_set.temporary?
            return
          end

          if result_set.size <= @current_offset
            @current_offset -= result_set.size
            result_set.close if result_set.temporary?
            return
          end

          @unsorted_result_sets << result_set if result_set.temporary?
          sort_keys = [
            {
              :key => @enumerator.shard_key_name,
              :order => @order,
            },
          ]
          sorted_result_set = result_set.sort(sort_keys,
                                              :offset => @current_offset,
                                              :limit => @current_limit)
          @result_sets << sorted_result_set
          if @current_offset > 0
            @current_offset = 0
          end
          @current_limit -= sorted_result_set.size
        end
      end
    end
  end
end
