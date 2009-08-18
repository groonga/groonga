%name grn_expr_parser
%token_prefix GRN_EXPR_TOKEN_
%include {
#define assert GRN_ASSERT
}

%token_type { int }

%extra_argument { efs_info *efsi }

%syntax_error {
  {
    grn_ctx *ctx = efsi->ctx;
    ERR(GRN_SYNTAX_ERROR, "Syntax error!");
  }
}

query ::= expression.

expression ::= assignment_expression.
expression ::= expression COMMA assignment_expression.

assignment_expression ::= conditional_expression.
assignment_expression ::= lefthand_side_expression ASSIGN assignment_expression.
assignment_expression ::= lefthand_side_expression STAR_ASSIGN assignment_expression.
assignment_expression ::= lefthand_side_expression SLASH_ASSIGN assignment_expression.
assignment_expression ::= lefthand_side_expression MOD_ASSIGN assignment_expression.
assignment_expression ::= lefthand_side_expression PLUS_ASSIGN assignment_expression.
assignment_expression ::= lefthand_side_expression MINUS_ASSIGN assignment_expression.
assignment_expression ::= lefthand_side_expression SHIFTL_ASSIGN assignment_expression.
assignment_expression ::= lefthand_side_expression SHIRTR_ASSIGN assignment_expression.
assignment_expression ::= lefthand_side_expression SHIFTRR_ASSIGN assignment_expression.
assignment_expression ::= lefthand_side_expression AND_ASSIGN assignment_expression.
assignment_expression ::= lefthand_side_expression XOR_ASSIGN assignment_expression.
assignment_expression ::= lefthand_side_expression OR_ASSIGN assignment_expression.

conditional_expression ::= logical_or_expression.
conditional_expression ::= logical_or_expression QUESTION assignment_expression COLON assignment_expression.

logical_or_expression ::= logical_and_expression.
logical_or_expression ::= logical_or_expression LOGICAL_OR logical_and_expression. {
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_OR, 2);
}

logical_and_expression ::= bitwise_or_expression.
logical_and_expression ::= logical_and_expression LOGICAL_AND bitwise_or_expression. {
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND, 2);
}
logical_and_expression ::= logical_and_expression LOGICAL_BUT bitwise_or_expression. {
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BUT, 2);
}

bitwise_or_expression(A) ::= bitwise_xor_expression(B). {
  if (!B) {
    grn_obj *column, *token;
    GRN_PTR_POP(&efsi->token_stack, token);
    column = grn_ptr_value_at(&efsi->column_stack, -1);
    grn_expr_append_obj(efsi->ctx, efsi->e, efsi->v);
    grn_expr_append_const(efsi->ctx, efsi->e, column);
    grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_OBJ_GET_VALUE, 2);
    grn_expr_append_code(efsi->ctx, (grn_expr *)efsi->e, token, GRN_OP_PUSH);
    grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MATCH, 2);
  }
  A = 1;
}
bitwise_or_expression(A) ::= bitwise_or_expression(B) BITWISE_OR bitwise_xor_expression. { A = B;}

bitwise_xor_expression(A) ::= bitwise_and_expression(B). { A = B;}
bitwise_xor_expression(A) ::= bitwise_xor_expression(B) BITWISE_XOR bitwise_and_expression. { A = B;}

bitwise_and_expression(A) ::= equality_expression(B). { A = B;}
bitwise_and_expression(A) ::= bitwise_and_expression(B) BITWISE_AND equality_expression. { A = B;}

equality_expression(A) ::= relational_expression(B). { A = B;}
equality_expression(A) ::= equality_expression(B) EQUAL relational_expression. { A = B;}
equality_expression(A) ::= equality_expression(B) NOT_EQUAL relational_expression. { A = B;}

relational_expression(A) ::= shift_expression(B). { A = B;}
relational_expression(A) ::= relational_expression(B) LESS shift_expression. { A = B;}
relational_expression(A) ::= relational_expression(B) GREATER shift_expression. { A = B;}
relational_expression(A) ::= relational_expression(B) LESS_EQUAL shift_expression. { A = B;}
relational_expression(A) ::= relational_expression(B) GREATER_EQUAL shift_expression. { A = B;}
relational_expression(A) ::= relational_expression(B) IN shift_expression. { A = B;}
relational_expression(A) ::= relational_expression(B) MATCH shift_expression. { A = B;}

shift_expression(A) ::= additive_expression(B). { A = B;}
shift_expression(A) ::= shift_expression(B) SHIFTL additive_expression. { A = B;}
shift_expression(A) ::= shift_expression(B) SHIFTR additive_expression. { A = B;}
shift_expression(A) ::= shift_expression(B) SHIFTRR additive_expression. { A = B;}

additive_expression(A) ::= multiplicative_expression(B). { A = B;}
additive_expression(A) ::= additive_expression(B) PLUS multiplicative_expression. { A = B;}
additive_expression(A) ::= additive_expression(B) MINUS multiplicative_expression. { A = B;}

multiplicative_expression(A) ::= unary_expression(B). { A = B;}
multiplicative_expression(A) ::= multiplicative_expression(B) STAR unary_expression. { A = B;}
multiplicative_expression(A) ::= multiplicative_expression(B) SLASH unary_expression. { A = B;}
multiplicative_expression(A) ::= multiplicative_expression(B) MOD unary_expression. { A = B;}

unary_expression(A) ::= postfix_expression(B). { A = B;}
unary_expression(A) ::= DELETE unary_expression(B). { A = B;}
unary_expression(A) ::= INCR unary_expression(B). { A = B;}
unary_expression(A) ::= DECR unary_expression(B). { A = B;}
unary_expression(A) ::= PLUS unary_expression(B). { A = B;}
unary_expression(A) ::= MINUS unary_expression(B). { A = B;}
unary_expression(A) ::= NOT unary_expression(B). { A = B;}
unary_expression(A) ::= ADJ_INC unary_expression(B). { A = B;}
unary_expression(A) ::= ADJ_DEC unary_expression(B). { A = B;}
unary_expression(A) ::= ADJ_NEG unary_expression(B). { A = B;}
unary_expression(A) ::= UNARY_SIMILAR unary_expression(B). { A = B;}
unary_expression(A) ::= UNARY_EXTRACT unary_expression(B). { A = B;}
unary_expression(A) ::= UNARY_NEAR unary_expression(B). { A = B;}

postfix_expression(A) ::= lefthand_side_expression(B). { A = B;}
postfix_expression(A) ::= lefthand_side_expression(B) INCR. { A = B;}
postfix_expression(A) ::= lefthand_side_expression(B) DECR. { A = B;}

lefthand_side_expression(A) ::= call_expression(B). { A = B;}
lefthand_side_expression(A) ::= member_expression(B). { A = B;}

call_expression(A) ::= member_expression arguments(B). { A = B;}

member_expression(A) ::= primary_expression(B). { A = B;}
member_expression(A) ::= member_expression(B) member_expression_part. { A = B;}

primary_expression ::= object_literal.
primary_expression(A) ::= PARENL expression(B) PARENR. { A = 1;}
primary_expression ::= IDENTIFIER.
primary_expression ::= array_literal.
primary_expression ::= DECIMAL.
primary_expression ::= HEX_INTEGER.
primary_expression ::= STRING.
primary_expression ::= BOOLEAN.
primary_expression ::= NULL.

array_literal ::= BRACKETL elision BRACKETR.
array_literal ::= BRACKETL element_list elision BRACKETR.
array_literal ::= BRACKETL element_list BRACKETR.

elision ::= COMMA.
elision ::= elision COMMA.

element_list ::= assignment_expression.
element_list ::= elision assignment_expression.
element_list ::= element_list elision assignment_expression.

object_literal ::= BRACEL property_name_and_value_list RBRACE.

property_name_and_value_list ::= .
property_name_and_value_list ::= property_name_and_value_list COMMA property_name_and_value.

property_name_and_value ::= property_name COLON assignment_expression.
property_name ::= IDENTIFIER|STRING|DECIMAL.

member_expression_part ::= BRACKETL expression BRACKETR.
member_expression_part ::= DOT IDENTIFIER.

arguments ::= PARENL argument_list PARENR.
argument_list ::= .
argument_list ::= argument_list COMMA assignment_expression.
