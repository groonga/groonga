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
                 "dependent",
               ])

      def run_body(input)
        @dependent = (input[:dependent] == "yes")

        enumerator = LogicalEnumerator.new("logical_table_remove", input)

        success = true
        enumerator.each do |shard, shard_range|
          remove_shard(shard, shard_range, enumerator.target_range)
        end
        writer.write(success)
      end

      private
      def remove_shard(shard, shard_range, target_range)
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
          remove_table(shard)
        when :partial_min
          remove_records(table) do |expression|
            expression_builder.build_partial_min(expression)
          end
          remove_table(shard) if table.empty?
        when :partial_max
          remove_records(table) do |expression|
            expression_builder.build_partial_max(expression)
          end
          remove_table(shard) if table.empty?
        when :partial_min_and_max
          remove_records(table) do |expression|
            expression_builder.build_partial_min_and_max(expression)
          end
          remove_table(shard) if table.empty?
        end
      end

      def remove_table(shard)
        table = shard.table
        shard_suffix = shard.range_data.to_suffix

        referenced_table_ids = []
        if @dependent
          table.columns.each do |column|
            range = column.range
            if range.is_a?(Table)
              referenced_table_ids << range.id
              range.indexes.each do |index_info|
                referenced_table_ids << index_info.index.domain.id
              end
            end
            column.indexes.each do |index_info|
              referenced_table_ids << index_info.index.domain.id
            end
          end
        end

        table.remove(:dependent => @dependent)

        context = Groonga::Context.instance
        referenced_table_ids.each do |referenced_table_id|
          referenced_table = context[referenced_table_id]
          next if referenced_table.nil?
          if referenced_table.name.end_with?(shard_suffix)
            referenced_table.remove(:dependent => @dependent)
          end
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
