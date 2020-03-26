module Groonga
  class ScanInfoDataSizeEstimator
    def initialize(data, table)
      @data = data
      @table = table
      @table_size = @table.size
    end

    def estimate
      search_index = @data.search_indexes.first
      return @table_size if search_index.nil?

      index_column = resolve_index_column(search_index.index_column)
      return @table_size if index_column.nil?

      size = nil
      case @data.op
      when Operator::CALL
        size = estimate_call(index_column)
      else
        left = ExpressionTree::Variable.new(index_column)
        right = ExpressionTree::Constant.new(@data.query)
        node = ExpressionTree::BinaryOperation.new(@data.op, left, right)
        size = node.estimate_size(@table)
      end
      index_column.unlink
      size || @table_size
    end

    private
    def resolve_index_column(index_column)
      index_column.refer

      while index_column.is_a?(Accessor)
        index_info = index_column.find_index(@data.op)
        if index_info.nil?
          index_column.unlink
          return nil
        end
        index_column.unlink
        break if index_info.index == index_column
        index_column = index_info.index
      end

      index_column
    end

    def estimate_call(index_column)
      procedure = @data.args[0]
      arguments = @data.args[1..-1].collect do |arg|
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
