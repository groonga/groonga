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
          index_column = search_index.index_column
          if index_column.is_a?(Accessor)
            # TODO
            @table.size
          else
            index_column.estimate_size(:query => data.query.value)
          end
        end
      end
      sizes.min
    end
  end
end
