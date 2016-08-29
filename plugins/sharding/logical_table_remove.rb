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
                 "force",
               ])

      def run_body(input)
        @dependent = (input[:dependent] == "yes")
        @force = (input[:force] == "yes")

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

        if cover_type == :all or (table.nil? and @force)
          remove_table(shard, table)
          return
        end

        expression_builder = RangeExpressionBuilder.new(shard_key,
                                                        target_range,
                                                        nil)
        case cover_type
        when :partial_min
          remove_records(table) do |expression|
            expression_builder.build_partial_min(expression)
          end
          remove_table(shard, table) if table.empty?
        when :partial_max
          remove_records(table) do |expression|
            expression_builder.build_partial_max(expression)
          end
          remove_table(shard, table) if table.empty?
        when :partial_min_and_max
          remove_records(table) do |expression|
            expression_builder.build_partial_min_and_max(expression)
          end
          remove_table(shard, table) if table.empty?
        end
      end

      def remove_table(shard, table)
        if table.nil? and @force
          context.clear_error
        end

        referenced_table_ids = []
        if @dependent
          if table
            columns = table.columns
          else
            prefix = "#{shard.table_name}."
            columns = []
            context.database.each_name(:prefix => prefix) do |column_name|
              column = context[column_name]
              columns << column
              context.clear_error if column.nil?
            end
          end
          columns.each do |column|
            next if column.nil?
            range = column.range
            case range
            when nil
              context.clear_error
            when Table
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

        if table.nil?
          if @force
            remove_table_force(shard.table_name)
          else
            message = "[logical_table_remove] table is broken: " +
                      "<#{shard.table_name}>"
            raise InvalidArgument, message
          end
        else
          options = {:dependent => @dependent}
          if @force
            begin
              table.remove(options)
            rescue
              table.close
              remove_table_force(shard.table_name)
            end
          else
            table.remove(options)
          end
        end

        return if referenced_table_ids.empty?

        shard_suffix = shard.range_data.to_suffix
        referenced_table_ids.each do |referenced_table_id|
          referenced_table = context[referenced_table_id]
          if referenced_table.nil?
            context.clear_error
            if @force
              Object.remove_force(referenced_table_id)
            end
            next
          end

          referenced_table_name = referenced_table.name
          next unless referenced_table_name.end_with?(shard_suffix)

          if @force
            begin
              referenced_table.remove(:dependent => @dependent)
            rescue
              Context.instance.clear_error
              referenced_table.close
              remove_table_force(referenced_table_name)
            end
          else
            referenced_table.remove(:dependent => @dependent)
          end
        end
      end

      def remove_table_force(table_name)
        database = context.database

        prefix = "#{table_name}."
        database.each_raw(:prefix => prefix) do |id, cursor|
          column = context[id]
          if column.nil?
            context.clear_error
            column_name = cursor.key
            Object.remove_force(column_name)
          else
            column.remove(:dependent => @dependent)
          end
        end

        table_id = database[table_name]
        return if table_id.nil?

        database.each_raw do |id, cursor|
          next if id == table_id
          object = context[id]
          case object
          when Table
            if object.domain_id == table_id
              object.remove(:dependent => @dependent)
            end
          when Column
            if object.range_id == table_id
              object.remove(:dependent => @dependent)
            end
          when nil
            context.clear_error
          end
        end

        Object.remove_force(table_name)
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
