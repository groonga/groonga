%name grn_expr_parser

%include {
#define assert GRN_ASSERT
}

%token_type {int}

%left PLUS MINUS.
%left DIVIDE TIMES.

%syntax_error {
  puts("Syntax error!");
}

program ::= expr(A).   { printf("Result=%d\n", A); }

expr(A) ::= expr(B) MINUS  expr(C).   { A = B - C; }
expr(A) ::= expr(B) PLUS  expr(C).   { A = B + C; }
expr(A) ::= expr(B) TIMES  expr(C).   { A = B * C; }
expr(A) ::= expr(B) DIVIDE expr(C).  {
         if (C != 0) {
           A = B / C;
         } else {
           puts("divide by zero");
         }
}
