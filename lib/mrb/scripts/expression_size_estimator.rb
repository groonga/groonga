module Groonga
  class ExpressionSizeEstimator
    def initialize(expression, table)
      @expression = expression
      @table = table
    end

    def estimate
      builder = ScanInfoBuilder.new(@expression, Operator::OR, 0)
      data_list = builder.build
      return @table.size if data_list.nil?

      sizes = data_list.collect do |data|
        search_index = data.search_indexes.first
        if search_index.nil?
          @table.size
        else
          case data.op
          when Operator::MATCH
            estimate_match(data, search_index) || @table.size
          when Operator::LESS
            estimate_less(data, search_index) || @table.size
          else
            @table.size
          end
        end
      end
      sizes.min
    end

    private
    def estimate_match(data, search_index)
      index_column = search_index.index_column
      if index_column.is_a?(Accessor)
        # TODO
        return nil
      end

      index_column.estimate_size(:query => data.query.value)
    end

    def estimate_less(data, search_index)
      index_column = search_index.index_column
      if index_column.is_a?(Accessor)
        # TODO
        return nil
      end

      lexicon = index_column.lexicon
      max = data.query.value
      flags = TableCursorFlags::LT
      TableCursor.open(lexicon, :max => max, :flags => flags) do |cursor|
        index_column.estimate_size(:lexicon_cursor => cursor)
      end
    end
  end
end
