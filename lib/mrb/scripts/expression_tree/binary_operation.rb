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
        Operator::NEAR_PHRASE,
        Operator::ORDERED_NEAR_PHRASE,
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
        case column
        when Groonga::Accessor
          accessor = column
          term = value.value
          return column.estimate_size(query: term,
                                      mode: Operator::EQUAL)
        when Groonga::IndexColumn
          index_column = column
        else
          index_info = column.find_index(@operator)
          return table.size if index_info.nil?
          index_column = index_info.index
        end
        return table.size unless index_column.respond_to?(:lexicon)

        case value
        when Groonga::Vector, Groonga::UVector
          return table.size
        end

        lexicon = index_column.lexicon
        term = value.value
        if value.domain_id == lexicon.id
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

        is_table_search = false
        column = @left.column
        value = @right.value
        case column
        when Table
          is_table_search = true
          lexicon = column
        when Groonga::Accessor
          is_table_search = true
          lexicon = column.object
          case lexicon
          when PatriciaTrie, DoubleArrayTrie
            # range searchable
          else
            return table.size
          end
        when Groonga::IndexColumn
          index_column = column
          lexicon = column.lexicon
        else
          index_info = column.find_index(@operator)
          return table.size if index_info.nil?

          index_column = index_info.index
          lexicon = index_column.lexicon
        end
        n_terms = lexicon.size
        return 0 if n_terms.zero?

        options = {
          limit: sampling_cursor_limit(n_terms),
        }
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
          if is_table_search
            size = 1
          else
            size = index_column.estimate_size(:lexicon_cursor => cursor)
          end
          size += 1 if cursor.next != ID::NIL
          size
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
        when Groonga::IndexColumn
          indexes << column
        when Groonga::Accessor
          indexes << column
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

        is_table_search = false
        column = @left.column
        case column
        when Groonga::Accessor
          return table.size unless column.key?
          is_table_search = true
          lexicon = column.object
        when Groonga::IndexColumn
          lexicon = column.lexicon
        else
          return table.size
        end
        n_terms = lexicon.size
        return 0 if n_terms.zero?

        value = @right.value
        options = {
          min: value,
          limit: sampling_cursor_limit(n_terms),
          flags: TableCursorFlags::PREFIX,
        }
        TableCursor.open(lexicon, options) do |cursor|
          if is_table_search
            size = 1
          else
            size = column.estimate_size(:lexicon_cursor => cursor)
          end
          size += 1 if cursor.next != ID::NIL
          size
        end
      end

      def sampling_cursor_limit(n_terms)
        limit = n_terms * 0.01
        if limit < 10
          10
        elsif limit > 1000
          1000
        else
          limit.to_i
        end
      end
    end
  end
end
