module Groonga
  module ExpressionTree
    class BinaryOperation
      attr_reader :operator
      attr_reader :left
      attr_reader :right
      attr_reader :parameter
      attr_reader :value
      def initialize(operator, left, right, options={})
        @operator = operator
        @left = left
        @right = right
        @parameter = options[:parameter]
        @value = options[:value]
      end

      def build(expression)
        @left.build(expression)
        @right.build(expression)
        n_args = 2
        if @parameter
          @parameter.build(expression)
          n_args += 1
        end
        if @value
          expression.append_constant(@value, @operator, n_args)
        else
          expression.append_operator(@operator, n_args)
        end
      end

      def estimatable?
        true
      end

      RANGE_OPERATORS = [
        Operator::LESS,
        Operator::GREATER,
        Operator::LESS_EQUAL,
        Operator::GREATER_EQUAL,
      ]
      QUERY_OPERATIONS = [
        Operator::MATCH,
        Operator::NEAR,
        Operator::NEAR2,
        Operator::SIMILAR,
        Operator::EXACT,
        Operator::REGEXP,
        Operator::FUZZY,
        Operator::QUORUM,
      ]
      def estimate_size(table)
        case @operator
        when Operator::EQUAL
          estimate_size_equal(table)
        when *RANGE_OPERATORS
          estimate_size_range(table)
        when *QUERY_OPERATIONS
          estimate_size_query(table)
        when Operator::PREFIX
          estimate_size_prefix(table)
        else
          table.size
        end
      end

      private
      def estimate_size_equal(table)
        return table.size unless @left.is_a?(Variable)
        return table.size unless @right.is_a?(Constant)

        column = @left.column
        value = @right.value
        index_info = column.find_index(@operator)
        return table.size if index_info.nil?

        index_column = index_info.index
        return table.size unless index_column.respond_to?(:lexicon)

        lexicon = index_column.lexicon
        term = value.value
        if value.domain == lexicon.id
          term_id = term.id
        else
          term_id = lexicon[term]
        end
        if term_id.nil?
          0
        else
          index_column.estimate_size(term_id: term_id)
        end
      end

      def estimate_size_range(table)
        return table.size unless @left.is_a?(Variable)
        return table.size unless @right.is_a?(Constant)

        column = @left.column
        value = @right.value
        index_info = column.find_index(@operator)
        return table.size if index_info.nil?

        index_column = index_info.index
        lexicon = index_column.lexicon
        options = {}
        case @operator
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

      def estimate_size_query(table)
        return table.size unless @left.is_a?(Variable)
        return table.size unless @right.is_a?(Constant)

        column = @left.column
        indexes = []
        case column
        when Expression
          column.codes.each do |code|
            case code.op
            when Operator::GET_VALUE
              value = code.value
              case value
              when IndexColumn
                indexes << index
              when Column
                index_info = value.find_index(@operator)
                return table.size if index_info.nil?
                indexes << index_info.index
              end
            end
          end
        else
          index_info = column.find_index(@operator)
          return table.size if index_info.nil?
          indexes << index_info.index
        end

        return table.size if indexes.empty?

        value = @right.value
        sizes = indexes.collect do |index|
          index.estimate_size(:query => value,
                              :mode => @operator)
        end
        sizes.max
      end

      def estimate_size_prefix(table)
        return table.size unless @left.is_a?(Variable)
        return table.size unless @right.is_a?(Constant)

        key = @left.column
        return table.size unless key.is_a?(Groonga::Accessor)
        return table.size unless key.key?

        # TODO: Improve
        table.size / 10
      end
    end
  end
end
