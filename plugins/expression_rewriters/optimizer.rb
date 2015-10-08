module Groonga
  module ExpressionRewriters
    class Optimizer < ExpressionRewriter
      register "optimizer"

      def rewrite
        codes = @expression.codes
        n_codes = codes.size

        # (A >= x && A < y) -> between(A, x, "include", y, "exclude")
        return nil if n_codes != 7

        return nil if codes[6].op != Operator::AND

        return nil if codes[0].op != Operator::GET_VALUE
        return nil if codes[1].op != Operator::PUSH
        return nil if codes[2].op != Operator::GREATER_EQUAL

        return nil if codes[3].op != Operator::GET_VALUE
        return nil if codes[4].op != Operator::PUSH
        return nil if codes[5].op != Operator::LESS

        return nil if codes[3].value != codes[0].value

        variable = @expression[0]
        rewritten = Expression.create(context[variable.domain])
        rewritten.append_object(Context.instance["between"], Operator::PUSH, 1)
        rewritten.append_object(codes[0].value, Operator::GET_VALUE, 1)
        rewritten.append_constant(codes[1].value.value, Operator::PUSH, 1)
        rewritten.append_constant("include", Operator::PUSH, 1)
        rewritten.append_constant(codes[4].value.value, Operator::PUSH, 1)
        rewritten.append_constant("exclude", Operator::PUSH, 1)
        rewritten.append_operator(Operator::CALL, 5)
        rewritten
      end
    end
  end
end
