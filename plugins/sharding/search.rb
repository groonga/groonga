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

        total = 0
        context.database.each_table(:prefix => "#{logical_table}_") do |table|
          total += table.size
        end
        output(total)
      end
    end
  end
end
