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
        filter = input[:filter]

        total = 0
        enumerator.each do |table, shard_key, shard_range|
          total += count_n_records(table, filter,
                                   shard_key, shard_range,
                                   enumerator.target_range)
        end
        writer.write(total)
      end

      private
      def count_n_records(table, filter,
                          shard_key, shard_range,
                          target_range)
        cover_type = target_range.cover_type(shard_range)
        return 0 if cover_type == :none

        expression_builder = RangeExpressionBuilder.new(shard_key,
                                                        target_range,
                                                        filter)
        if cover_type == :all
          if filter.nil?
            return table.size
          else
            return filtered_count_n_records(table, filter) do |expression|
              expression_builder.build_all(expression)
            end
          end
        end

        case cover_type
        when :partial_min
          filtered_count_n_records(table, filter) do |expression|
            expression_builder.build_partial_min(expression)
          end
        when :partial_max
          filtered_count_n_records(table, filter) do |expression|
            expression_builder.build_partial_max(expression)
          end
        when :partial_min_and_max
          filtered_count_n_records(table, filter) do |expression|
            expression_builder.build_partial_min_and_max(expression)
          end
        end
      end

      def filtered_count_n_records(table, filter)
        expression = nil
        filtered_table = nil

        begin
          expression = Expression.create(table)
          yield(expression)
          filtered_table = table.select(expression)
          filtered_table.size
        ensure
          filtered_table.close if filtered_table
          expression.close if expression
        end
      end
    end
  end
end
