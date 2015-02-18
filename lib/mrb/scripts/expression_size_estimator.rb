module Groonga
  class ExpressionSizeEstimator
    def initialize(expression, table)
      @expression = expression
      @table = table
    end

    def estimate
      @table.size
    end
  end
end
