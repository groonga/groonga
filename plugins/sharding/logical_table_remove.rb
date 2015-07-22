module Groonga
  module Sharding
    class LogicalTableRemoveCommand < Command
      register("logical_table_remove",
               [
                 "logical_table",
                 "shard_key",
                 "min",
                 "min_border",
                 "max",
                 "max_border",
               ])

      def run_body(input)
        enumerator = LogicalEnumerator.new("logical_table_remove", input)

        succeess = true
        enumerator.each do |shard, shard_range|
          remove_table(shard, shard_range, enumerator.target_range)
        end
        writer.write(succeess)
      end

      private
      def remove_table(shard, shard_range, target_range)
        cover_type = target_range.cover_type(shard_range)
        return if cover_type == :none

        shard_key = shard.key
        if shard_key.nil?
          message = "[logical_table_remove] shard_key doesn't exist: " +
                    "<#{shard.key_name}>"
          raise InvalidArgument, message
        end
        table = shard.table

        expression_builder = RangeExpressionBuilder.new(shard_key,
                                                        target_range,
                                                        nil)

        case cover_type
        when :all
          table.remove
        when :partial_min
          remove_records(table) do |expression|
            expression_builder.build_partial_min(expression)
          end
          table.remove if table.empty?
        when :partial_max
          remove_records(table) do |expression|
            expression_builder.build_partial_max(expression)
          end
          table.remove if table.empty?
        when :partial_min_and_max
          remove_records(table) do |expression|
            expression_builder.build_partial_min_and_max(expression)
          end
          table.remove if table.empty?
        end
      end

      def remove_records(table)
        expression = nil

        begin
          expression = Expression.create(table)
          yield(expression)
          table.delete(:expression => expression)
        ensure
          expression.close if expression
        end
      end
    end
  end
end
