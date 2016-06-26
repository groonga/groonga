module Groonga
  module ExpressionTree
    class Accessor
      attr_reader :accessor
      def initialize(accessor)
        @accessor = accessor
      end

      def build(expression)
        expression.append_object(@accessor, Operator::PUSH, 1)
      end
    end
  end
end
