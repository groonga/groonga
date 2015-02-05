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
        enumerator = LogicalEnumerator.new("logical_range_filter", input)
        filter = input[:filter]
        offset = input[:offset] || 0
        limit = input[:limit] || 10
        output_columns = input[:output_columns] || "_key, *"

        result_sets = []
        enumerator.each do |table, shard_key, shard_range, cover_type|
          # TODO: result_sets << result_set
        end
        if result_sets.empty?
          n_elements = 0
        else
          n_elements = 1 # for columns
          result_sets.each do |result_set|
            n_elements += result_size.size
          end
        end
        writer.array("RESULTSET", n_elements) do
          first_result_set = result_sets.first
          if first_result_set
            # TODO: write columns of first_result_set
          end
          result_sets.each do |result_set|
            # TODO: write records
          end
        end
      end
    end
  end
end
