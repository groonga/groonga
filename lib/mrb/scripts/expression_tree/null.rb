module Groonga
  module ExpressionTree
    class Null
      def build(expression)
        expression.append_constant(nil, Operator::PUSH, 1)
      end

      def estimatable?
        true
      end

      def estimate_size(table)
        0
      end
    end
  end
end
