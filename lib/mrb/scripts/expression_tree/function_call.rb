module Groonga
  module ExpressionTree
    class FunctionCall
      attr_reader :procedure
      attr_reader :arguments
      def initialize(procedure, arguments)
        @procedure = procedure
        @arguments = arguments
      end

      def build(expression)
        @procedure.build(expression)
        @arguments.each do |argument|
          argument.build(expression)
        end
        expression.append_operator(Operator::CALL, @arguments.size)
      end

      def estimatable?
        true
      end

      def estimate_size(table)
        case @procedure.name
        when "between"
          estimate_size_between(table)
        when "in_values"
          estimate_size_in_values(table)
        when "query"
          estimate_size_query(table)
        when "query_parallel_or"
          estimate_size_query_parallel_or(table)
        else
          table.size
        end
      end

      private
      def estimate_size_between(table)
        column, min, min_border, max, max_border = @arguments

        if column.is_a?(Groonga::ExpressionTree::IndexColumn)
          index_column = column.object
        else
          index_info = column.column.find_index(Operator::CALL)
          return table.size if index_info.nil?
          index_column = index_info.index
        end

        while index_column.is_a?(Groonga::Accessor)
          if index_column.have_next?
            index_column = index_column.next
          else
            index_column = index_column.object
            index_info = index_column.find_index(Operator::CALL)
            return table.size if index_info.nil?
            index_column = index_info.index
          end
        end

        lexicon = index_column.lexicon
        options = {
          :min => min.value,
          :max => max.value,
          :flags => 0,
        }
        if min_border.value == "include"
          options[:flags] |= TableCursorFlags::LT
        else
          options[:flags] |= TableCursorFlags::LE
        end
        if max_border.value == "include"
          options[:flags] |= TableCursorFlags::GT
        else
          options[:flags] |= TableCursorFlags::GE
        end

        TableCursor.open(lexicon, options) do |cursor|
          index_column.estimate_size(:lexicon_cursor => cursor)
        end
      end

      def estimate_size_in_values(table)
        column, *values = @arguments
        estimated_sizes = values.collect do |value|
          node = BinaryOperation.new(Operator::EQUAL, column, value)
          node.estimate_size(table)
        end
        resolve_estimated_sizes(table, estimated_sizes)
      end

      def estimate_size_query_raw(table, match_columns, query)
        estimated_sizes = []
        expression = Expression.create(table)
        begin
          expression.parse(match_columns)
          expression.codes.each do |code|
            case code.op
            when Operator::PUSH
              value = code.value
              case value
              when ::Groonga::IndexColumn
                estimated_size = value.estimate_size(query: query)
              else
                next
              end
            when Operator::GET_VALUE
              index_info = code.value.find_index(Operator::MATCH)
              next unless index_info
              index = index_info.index
              estimated_size = index.estimate_size(query: query)
              index.unref
            else
              next
            end
            estimated_sizes << estimated_size
          end
        ensure
          expression.close
        end
        resolve_estimated_sizes(table, estimated_sizes)
      end

      def estimate_size_query(table)
        match_columns, query, = @arguments
        estimate_size_query_raw(table,
                                match_columns.value.value,
                                query.value)
      end

      def estimate_size_query_parallel_or(table)
        match_columns, *queries = @arguments
        queries.pop if queries.last.is_a?(HashTable)
        separated_queries = queries.collect do |query|
          "(#{query.value})"
        end
        estimate_size_query_raw(table,
                                match_columns.value.value,
                                separated_queries.join(" OR "))
      end

      def resolve_estimated_sizes(table, estimated_sizes)
        case estimated_sizes.size
        when 0
          return 0
        when 1
          estimated_size = estimated_sizes.first
        else
          total = estimated_sizes.inject(0) do |sum, size|
            sum + size
          end
          duplicated_ratio = 0.3 # No reason :p
          estimated_size = (total * (1 - duplicated_ratio)).round
        end
        n_records = table.size
        if estimated_size < n_records
          estimated_size
        else
          n_records
        end
      end
    end
  end
end
