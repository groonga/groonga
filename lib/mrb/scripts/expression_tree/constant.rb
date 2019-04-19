module Groonga
  module ExpressionTree
    class Constant
      attr_reader :value
      def initialize(value)
        # Groonga::Bulk, Groonga::Vector or raw Ruby object.
        # Should we unify to Groonga::*?
        @value = value
      end

      def build(expression)
        expression.append_constant(@value, Operator::PUSH, 1)
      end

      def estimatable?
        true
      end

      def estimate_size(table)
        if true_value?
          table.size
        else
          0
        end
      end

      private
      def true_value?
        if @value.respond_to?(:true?)
          @value.true?
        else
          Bulk.true?(@value)
        end
      end
    end
  end
end
