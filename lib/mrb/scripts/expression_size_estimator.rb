module Groonga
  class ExpressionSizeEstimator
    def initialize(expression, table)
      @expression = expression
      @table = table
      @table_size = @table.size
    end

    def estimate
      builder = ScanInfoBuilder.new(@expression, Operator::OR, false)
      data_list = builder.build
      return @table_size if data_list.nil?

      current_size = 0
      sizes = []
      data_list.each do |data|
        if (data.flags & ScanInfo::Flags::POP) != 0
          size = sizes.pop
          case data.logical_op
          when Operator::AND, Operator::AND_NOT
            current_size = size if size < current_size
          when Operator::OR
            current_size = size if size > current_size
          else
            message = "invalid logical operator: <#{data.logical_op.inspect}>"
            raise InvalidArgument, message
          end
        else
          if (data.flags & ScanInfo::Flags::PUSH) != 0
            sizes.push(current_size)
            current_size = 0
          end

          size = estimate_data(data)
          case data.logical_op
          when Operator::AND
            current_size = size if size < current_size
          when Operator::AND_NOT
            size = @table_size - size
            size = 0 if size < 0
            current_size = size if size < current_size
          when Operator::OR
            current_size = size if size > current_size
          else
            message = "invalid logical operator: <#{data.logical_op.inspect}>"
            raise InvalidArgument, message
          end
        end
      end
      current_size
    end

    private
    def estimate_data(data)
      search_index = data.search_indexes.first
      return @table_size if search_index.nil?

      index_column = resolve_index_column(search_index.index_column,
                                          data.op)
      return @table_size if index_column.nil?

      size = nil
      case data.op
      when Operator::MATCH,
           Operator::FUZZY
        size = estimate_match(data, index_column)
      when Operator::REGEXP
        size = estimate_regexp(data, index_column)
      when Operator::EQUAL
        size = estimate_equal(data, index_column)
      when Operator::LESS,
           Operator::LESS_EQUAL,
           Operator::GREATER,
           Operator::GREATER_EQUAL
        size = estimate_range(data, index_column)
      when Operator::PREFIX
        size = estimate_prefix(data, index_column)
      when Operator::CALL
        size = estimate_call(data, index_column)
      end
      size || @table_size
    end

    def resolve_index_column(index_column, operator)
      while index_column.is_a?(Accessor)
        index_info = index_column.find_index(operator)
        return nil if index_info.nil?
        index_column = index_info.index
      end

      index_column
    end

    def estimate_match(data, index_column)
      index_column.estimate_size(:query => data.query.value)
    end

    def estimate_regexp(data, index_column)
      index_column.estimate_size(:query => data.query.value,
                                 :mode => data.op)
    end

    def estimate_equal(data, index_column)
      lexicon = index_column.lexicon
      query = data.query
      if query.domain == lexicon.id
        term_id = query.value
      else
        term_id = lexicon[query]
      end
      return 0 if term_id.nil?

      index_column.estimate_size(:term_id => term_id)
    end

    def estimate_range(data, index_column)
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

    def estimate_prefix(data, index_column)
      lexicon = index_column.lexicon
      n_terms = lexicon.size
      return 0 if n_terms.zero?

      value = data.query.value
      limit = n_terms / 0.01
      if limit < 10
        limit = 10
      elsif limit > 1000
        limit = 1000
      end
      options = {
        :min => value,
        :limit => limit,
        :flags => TableCursorFlags::PREFIX,
      }
      TableCursor.open(lexicon, options) do |cursor|
        size = index_column.estimate_size(:lexicon_cursor => cursor)
        size += 1 if cursor.next != ID::NIL
        size
      end
    end

    def estimate_call(data, index_column)
      procedure = data.args[0]
      arguments = data.args[1..-1].collect do |arg|
        if arg.is_a?(::Groonga::Object)
          ExpressionTree::Variable.new(arg)
        else
          ExpressionTree::Constant.new(arg)
        end
      end
      node = ExpressionTree::FunctionCall.new(procedure, arguments)
      node.estimate_size(@table)
    end
  end
end
