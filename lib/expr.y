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

match_expression(A) ::= MATCH_OPERATOR match_expr_elements(B). {
  /* insert(B) */
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MATCH, 2);
  A = B + 1;
}
match_expression(A) ::= match_expr_elements(B). {
  /* insert(B); */
  grn_expr_append_op(efsi->ctx, efsi->e, efsi->default_mode, 2);
  A = B + 1;
}
match_expr_elements(A) ::= match_expr_element(B). { A = B; }
match_expr_elements(A) ::= match_expr_elements(B) match_expr_element(C). {
  grn_expr_append_op(efsi->ctx, efsi->e, efsi->default_op, 2);
  A = B + C + 1;
}
match_expr_elements(A) ::= match_expr_elements(B) OR match_expr_element(C). {
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_OR, 2);
  A = B + C + 1;
}
match_expr_elements(A) ::= match_expr_elements(B) AND match_expr_element(C). {
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND, 2);
  A = B + C + 1;
}
match_expr_elements(A) ::= match_expr_elements(B) BUT match_expr_element(C). {
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BUT, 2);
  A = B + C + 1;
}

match_expr_element(A) ::= word(B). { A = B; }
match_expr_element ::= ADJ_INC word. { puts(">"); }
match_expr_element ::= ADJ_DEC word. { puts("<"); }
match_expr_element ::= ADJ_NEG word. { puts("~"); }

word(A) ::= WORD|PHRASE(B). { A = B; }

word(A) ::= PARENL match_expr_elements(B) PARENR. { A = B; }
