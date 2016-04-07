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
        expression.append_object(@procedure, Operator::PUSH, 1)
        @arguments.each do |argument|
          argument.build(expression)
        end
        expression.append_operator(Operator::CALL, @arguments.size)
      end
    end
  end
end
