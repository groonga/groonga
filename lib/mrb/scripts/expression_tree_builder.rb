require "expression_tree"

module Groonga
  class ExpressionTreeBuilder
    RELATION_OPERATORS = [
      Operator::MATCH,
      Operator::NEAR,
      Operator::NEAR_NO_OFFSET,
      Operator::NEAR_PHRASE,
      Operator::ORDERED_NEAR_PHRASE,
      Operator::NEAR_PHRASE_PRODUCT,
      Operator::ORDERED_NEAR_PHRASE_PRODUCT,
      Operator::SIMILAR,
      Operator::PREFIX,
      Operator::SUFFIX,
      Operator::EQUAL,
      Operator::NOT_EQUAL,
      Operator::LESS,
      Operator::GREATER,
      Operator::LESS_EQUAL,
      Operator::GREATER_EQUAL,
      Operator::GEO_WITHINP5,
      Operator::GEO_WITHINP6,
      Operator::GEO_WITHINP8,
      Operator::TERM_EXTRACT,
      Operator::REGEXP,
      Operator::FUZZY,
      Operator::QUORUM,
    ]

    ARITHMETIC_OPERATORS = [
      Operator::BITWISE_OR,
      Operator::BITWISE_XOR,
      Operator::BITWISE_AND,
      Operator::SHIFTL,
      Operator::SHIFTR,
      Operator::SHIFTRR,
      Operator::PLUS,
      Operator::STAR,
      Operator::SLASH,
      Operator::MOD,
    ]

    LOGICAL_OPERATORS = [
      Operator::AND,
      Operator::OR,
      Operator::AND_NOT,
      Operator::ADJUST,
    ]

    UNARY_OPERATIONS = [
      Operator::NOT,
      Operator::BITWISE_NOT,
      Operator::INCR,
      Operator::DECR,
      Operator::INCR_POST,
      Operator::DECR_POST,
    ]

    ASSIGN_ARITHMETIC_OPERATIONS = [
      Operator::STAR_ASSIGN,
      Operator::SLASH_ASSIGN,
      Operator::MOD_ASSIGN,
      Operator::PLUS_ASSIGN,
      Operator::MINUS_ASSIGN,
      Operator::SHIFTL_ASSIGN,
      Operator::SHIFTR_ASSIGN,
      Operator::SHIFTRR_ASSIGN,
      Operator::AND_ASSIGN,
      Operator::XOR_ASSIGN,
      Operator::OR_ASSIGN,
    ]

    def initialize(expression)
      @expression = expression
    end

    def build
      stack = []
      codes = @expression.codes
      codes.each do |code|
        case code.op
        when *LOGICAL_OPERATORS
          right = stack.pop
          left = stack.pop
          nodes = []
          add_logical_operation_node(code.op, nodes, left)
          add_logical_operation_node(code.op, nodes, right)
          weight_factor = nil
          weight_factor = code.value.value if code.value
          node = ExpressionTree::LogicalOperation.new(code.op,
                                                      nodes,
                                                      weight_factor)
          stack.push(node)
        when *RELATION_OPERATORS, *ARITHMETIC_OPERATORS
          options = {}
          options[:parameter] = stack.pop if code.n_args == 3
          options[:value] = code.value.value if code.value
          right = stack.pop
          left = stack.pop
          node = ExpressionTree::BinaryOperation.new(code.op,
                                                     left,
                                                     right,
                                                     options)
          stack.push(node)
        when *UNARY_OPERATIONS
          value = stack.pop
          node = ExpressionTree::UnaryOperation.new(code.op, value)
          stack.push(node)
        when *ASSIGN_ARITHMETIC_OPERATIONS
          value = stack.pop
          variable = stack.pop
          node = ExpressionTree::AssignBinaryOperation.new(code.op,
                                                           variable,
                                                           value)
          stack.push(node)
        when Operator::MINUS
          if code.n_args == 1
            value = stack.pop
            node = ExpressionTree::UnaryOperation.new(code.op, value)
          else
            right = stack.pop
            left = stack.pop
            node = ExpressionTree::BinaryOperation.new(code.op, left, right)
          end
          stack.push(node)
        when Operator::GET_REF
          node = ExpressionTree::Reference.new(code.value)
          stack.push(node)
        when Operator::GET_VALUE
          node = ExpressionTree::Variable.new(code.value)
          stack.push(node)
        when Operator::GET_MEMBER
          index = stack.pop
          variable = stack.pop
          node = ExpressionTree::Member.new(variable, index)
          stack.push(node)
        when Operator::ASSIGN
          value = stack.pop
          variable = stack.pop
          node = ExpressionTree::Assign.new(variable, value)
          stack.push(node)
        when Operator::PUSH
          case code.value
          when Procedure
            node = ExpressionTree::Procedure.new(code.value)
          when IndexColumn
            node = ExpressionTree::IndexColumn.new(code.value)
          when Accessor
            node = ExpressionTree::Accessor.new(code.value)
          when Table
            node = ExpressionTree::Table.new(code.value)
          when Void
            node = ExpressionTree::Null.new
          else
            node = ExpressionTree::Constant.new(code.value)
          end
          stack.push(node)
        when Operator::CALL
          arguments = []
          (code.n_args - 1).times do
            arguments.unshift(stack.pop)
          end
          procedure = stack.pop
          node = ExpressionTree::FunctionCall.new(procedure, arguments)
          stack.push(node)
        else
          raise "unknown operator: #{code.inspect}"
        end
      end
      if stack.size == 1
        stack.pop
      else
        # Unsupported expression
        nil
      end
    end

    private
    def add_logical_operation_node(operator, nodes, node)
      if node.is_a?(ExpressionTree::LogicalOperation) and
          node.operator == operator and
          node.operator != Operator::AND_NOT
        nodes.concat(node.nodes)
      else
        nodes << node
      end
    end
  end
end
