module Groonga
  module Sharding
    class LogicalCountCommand < Command
      register("logical_count",
               [
                 "logical_table",
                 "shared_key",
                 "min",
                 "min_border",
                 "max",
                 "max_border",
                 "filter",
               ])

      def run_body(input)
        logical_table = input[:logical_table]
        if logical_table.nil?
          raise InvalidArgument, "[logical_count] logical_table is missing"
        end

        filter = input[:filter]

        total = 0
        context.database.each_table(:prefix => "#{logical_table}_") do |table|
          total += count_n_records(table, filter)
        end
        output(total)
      end

      private
      def count_n_records(table, filter)
        return table.size if filter.nil?

        expression = nil
        filtered_table = nil
        begin
          expression = Expression.create(table)
          expression.parse(filter)
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
