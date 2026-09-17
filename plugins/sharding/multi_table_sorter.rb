module Groonga
  module Sharding
    # Sorts records of multiple tables as one table.
    #
    # Records of different tables can't be put into one result set
    # because their IDs conflict.
    # So sort key values of all records are copied to one table with
    # their offsets and the table is sorted by `grn_table_sort()`.
    #
    # OPTIMIZE:
    # It's better that we implement this in C to avoid copying values.
    # But this implementation is enough for now because offset and limit
    # are small in most cases.
    class MultiTableSorter
      include KeysParsable

      TABLE_INDEX_COLUMN_NAME = "table_index"
      OFFSET_COLUMN_NAME = "offset"

      def initialize(tables, sort_keys)
        @tables = tables
        @sort_keys = sort_keys
      end

      def each(offset, limit, &block)
        union_records = Array.create("", nil)
        begin
          set_records(union_records)
          return if offset >= union_records.size

          sorted_table = union_records.sort(translate_sort_keys,
                                            offset: offset,
                                            limit: limit)
          begin
            each_sorted_record(sorted_table, &block)
          ensure
            sorted_table.close
          end
        ensure
          union_records.close
        end
      end

      private
      def sort_key_column_name(i)
        "sort_key#{i}"
      end

      def translate_sort_keys
        @sort_keys.each_with_index.collect do |sort_key, i|
          if sort_key_descending?(sort_key)
            "-#{sort_key_column_name(i)}"
          else
            sort_key_column_name(i)
          end
        end
      end

      def close_column(column)
        column.close if column.is_a?(Accessor)
      end

      def create_columns(union_records)
        flags = ObjectFlags::COLUMN_SCALAR
        uint32 = Context.instance["UInt32"]

        table_index_column = union_records.create_column(TABLE_INDEX_COLUMN_NAME, flags, uint32)
        offset_column = union_records.create_column(OFFSET_COLUMN_NAME, flags, uint32)

        sort_key_columns = []
        @sort_keys.each_with_index do |sort_key, i|
          source_column = @tables.first.find_column(sort_key_name(sort_key))
          begin
            sort_key_columns << union_records.create_column(sort_key_column_name(i),
                                                            flags,
                                                            source_column.range)
          ensure
            close_column(source_column)
          end
        end

        {
          table_index: table_index_column,
          offset: offset_column,
          sort_keys: sort_key_columns,
        }
      end

      def set_records(union_records)
        columns = create_columns(union_records)
        @tables.each_with_index do |table, index|
          source_columns = @sort_keys.collect do |sort_key|
            table.find_column(sort_key_name(sort_key))
          end
          begin
            record_offset = 0
            table.each do |source_id|
              id = union_records.add
              columns[:table_index][id] = index
              columns[:offset][id] = record_offset
              source_columns.each_with_index do |source_column, i|
                value = source_column[source_id]
                columns[:sort_keys][i][id] = value unless value.nil?
              end
              record_offset += 1
            end
          ensure
            source_columns.each do |source_column|
              close_column(source_column)
            end
          end
        end
      end

      def each_sorted_record(sorted_table)
        table_index_column = sorted_table.find_column(TABLE_INDEX_COLUMN_NAME)
        offset_column = sorted_table.find_column(OFFSET_COLUMN_NAME)
        begin
          sorted_table.each do |id|
            yield(table_index_column[id], offset_column[id])
          end
        ensure
          close_column(table_index_column)
          close_column(offset_column)
        end
      end
    end
  end
end
