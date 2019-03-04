module Groonga
  module ExpressionTree
    class Member
      attr_reader :variable
      attr_reader :index
      def initialize(variable, index)
        @variable = variable
        @index = index
      end

      def build(expression)
        @variable.build(expression)
        @index.build(expression)
        expression.append_operator(Operator::GET_MEMBER, 2)
      end
    end
  end
end
