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
          size = nil
          case data.op
          when Operator::MATCH
            size = estimate_match(data, search_index)
          when Operator::LESS,
               Operator::LESS_EQUAL,
               Operator::GREATER,
               Operator::GREATER_EQUAL
            size = estimate_range(data, search_index)
          when Operator::CALL
            procedure = data.args.first
            if procedure.is_a?(Procedure) and procedure.name == "between"
              size = estimate_between(data, search_index)
            end
          end
          size || @table.size
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

    def estimate_range(data, search_index)
      index_column = search_index.index_column
      if index_column.is_a?(Accessor)
        # TODO
        return nil
      end

      lexicon = index_column.lexicon
      value = data.query.value
      options = {}
      case data.op
      when Operator::LESS
        options[:max] = value
        options[:flags] = TableCursorFlags::LT
      when Operator::LESS_EQUAL
        options[:max] = value
        options[:flags] = TableCursorFlags::LE
      when Operator::GREATER
        options[:min] = value
        options[:flags] = TableCursorFlags::GT
      when Operator::GREATER_EQUAL
        options[:min] = value
        options[:flags] = TableCursorFlags::GE
      end
      TableCursor.open(lexicon, options) do |cursor|
        index_column.estimate_size(:lexicon_cursor => cursor)
      end
    end

    def estimate_between(data, search_index)
      index_column = search_index.index_column
      if index_column.is_a?(Accessor)
        # TODO
        return nil
      end

      lexicon = index_column.lexicon
      _, _, min, min_border, max, max_border = data.args
      options = {
        :min => min,
        :max => max,
        :flags => 0,
      }
      if min_border == "include"
        options[:flags] |= TableCursorFlags::LT
      else
        options[:flags] |= TableCursorFlags::LE
      end
      if max_border == "include"
        options[:flags] |= TableCursorFlags::GT
      else
        options[:flags] |= TableCursorFlags::GE
      end

      TableCursor.open(lexicon, options) do |cursor|
        index_column.estimate_size(:lexicon_cursor => cursor)
      end
    end
  end
end
