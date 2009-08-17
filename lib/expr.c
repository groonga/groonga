/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is include which follows the "include" declaration
** in the input file. */
#include <stdio.h>
#line 3 "expr.y"

#define assert GRN_ASSERT
#line 13 "expr.c"
/* Next is all token values, in a form suitable for use by makeheaders.
** This section will be null unless lemon is run with the -m switch.
*/
/* 
** These constants (all generated automatically by the parser generator)
** specify the various kinds of tokens (terminals) that the parser
** understands. 
**
** Each symbol here is a terminal symbol in the grammar.
*/
/* Make sure the INTERFACE macro is defined.
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/* The next thing included is series of defines which control
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 terminals
**                       and nonterminals.  "int" is used otherwise.
**    YYNOCODE           is a number of type YYCODETYPE which corresponds
**                       to no legal terminal or nonterminal number.  This
**                       number is used to fill in empty slots of the hash 
**                       table.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       have fall-back values which should be used if the
**                       original value of the token will not parse.
**    YYACTIONTYPE       is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 rules and
**                       states combined.  "int" is used otherwise.
**    grn_expr_parserTOKENTYPE     is the data type used for minor tokens given 
**                       directly to the parser from the tokenizer.
**    YYMINORTYPE        is the data type used for all minor tokens.
**                       This is typically a union of many types, one of
**                       which is grn_expr_parserTOKENTYPE.  The entry in the union
**                       for base tokens is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    grn_expr_parserARG_SDECL     A static variable declaration for the %extra_argument
**    grn_expr_parserARG_PDECL     A parameter declaration for the %extra_argument
**    grn_expr_parserARG_STORE     Code to store %extra_argument into yypParser
**    grn_expr_parserARG_FETCH     Code to extract %extra_argument from yypParser
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
*/
#define YYCODETYPE unsigned char
#define YYNOCODE 93
#define YYACTIONTYPE unsigned short int
#define grn_expr_parserTOKENTYPE  int 
typedef union {
  grn_expr_parserTOKENTYPE yy0;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define grn_expr_parserARG_SDECL  efs_info *efsi ;
#define grn_expr_parserARG_PDECL , efs_info *efsi 
#define grn_expr_parserARG_FETCH  efs_info *efsi  = yypParser->efsi 
#define grn_expr_parserARG_STORE yypParser->efsi  = efsi 
#define YYNSTATE 165
#define YYNRULE 98
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* The yyzerominor constant is used to initialize instances of
** YYMINORTYPE objects to zero. */
static const YYMINORTYPE yyzerominor;

/* Next are that tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N < YYNSTATE                  Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   YYNSTATE <= N < YYNSTATE+YYNRULE   Reduce by rule N-YYNSTATE.
**
**   N == YYNSTATE+YYNRULE              A syntax error has occurred.
**
**   N == YYNSTATE+YYNRULE+1            The parser accepts its input.
**
**   N == YYNSTATE+YYNRULE+2            No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.  
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
*/
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   140,  264,   76,  124,  102,   62,   77,   78,   95,   96,
 /*    10 */    97,   83,   65,   72,   87,   71,  138,  106,  121,   57,
 /*    20 */     8,  127,   23,  128,  132,   31,   32,   33,   34,   35,
 /*    30 */    36,    5,   46,   47,  140,  143,   98,   43,   44,   45,
 /*    40 */    48,   49,   50,   51,   52,   53,   54,    3,  149,  131,
 /*    50 */   133,  134,  135,  136,  137,    1,  139,   60,  145,  145,
 /*    60 */     6,  145,   42,   55,   56,   88,   46,   47,  149,  165,
 /*    70 */     6,   43,   44,   45,   48,   49,   50,   51,   52,   53,
 /*    80 */    54,    3,   89,  131,  133,  134,  135,  136,  137,    1,
 /*    90 */   146,   60,   37,   38,   39,  122,   26,  123,   24,   25,
 /*   100 */    46,   47,   29,   30,  148,   43,   44,   45,   48,   49,
 /*   110 */    50,   51,   52,   53,   54,    3,  125,  131,  133,  134,
 /*   120 */   135,  136,  137,    1,    9,   60,   40,   41,  119,  120,
 /*   130 */    27,   10,  150,  102,   62,   77,   78,   95,   96,   97,
 /*   140 */    83,   65,   72,   87,   71,  138,  106,  121,   57,   28,
 /*   150 */   127,  151,  128,  132,    4,   59,   46,   47,  265,  265,
 /*   160 */   265,   43,   44,   45,   48,   49,   50,   51,   52,   53,
 /*   170 */    54,    3,  265,  131,  133,  134,  135,  136,  137,    1,
 /*   180 */   265,   60,   82,  124,  102,   62,   77,   78,   95,   96,
 /*   190 */    97,   83,   65,   72,   87,   71,  138,  106,  121,   57,
 /*   200 */   265,  127,  265,  128,  132,   85,  124,  102,   62,   77,
 /*   210 */    78,   95,   96,   97,   83,   65,   72,   87,   71,  138,
 /*   220 */   106,  121,   57,  265,  127,  265,  128,  132,  141,  102,
 /*   230 */    62,   77,   78,   95,   96,   97,   83,   65,   72,   87,
 /*   240 */    71,  138,  106,  121,   57,  265,  127,  265,  128,  132,
 /*   250 */   147,  102,   62,   77,   78,   95,   96,   97,   83,   65,
 /*   260 */    72,   87,   71,  138,  106,  121,   57,  265,  127,  265,
 /*   270 */   128,  132,  101,  102,   62,   77,   78,   95,   96,   97,
 /*   280 */    83,   65,   72,   87,   71,  138,  106,  121,   57,  265,
 /*   290 */   127,  265,  128,  132,  103,  102,   62,   77,   78,   95,
 /*   300 */    96,   97,   83,   65,   72,   87,   71,  138,  106,  121,
 /*   310 */    57,  265,  127,  265,  128,  132,   91,  102,   62,   77,
 /*   320 */    78,   95,   96,   97,   83,   65,   72,   87,   71,  138,
 /*   330 */   106,  121,   57,  265,  127,  265,  128,  132,  104,  102,
 /*   340 */    62,   77,   78,   95,   96,   97,   83,   65,   72,   87,
 /*   350 */    71,  138,  106,  121,   57,  265,  127,  265,  128,  132,
 /*   360 */   144,  102,   62,   77,   78,   95,   96,   97,   83,   65,
 /*   370 */    72,   87,   71,  138,  106,  121,   57,  265,  127,  265,
 /*   380 */   128,  132,  153,  102,   62,   77,   78,   95,   96,   97,
 /*   390 */    83,   65,   72,   87,   71,  138,  106,  121,   57,  265,
 /*   400 */   127,  265,  128,  132,  154,  102,   62,   77,   78,   95,
 /*   410 */    96,   97,   83,   65,   72,   87,   71,  138,  106,  121,
 /*   420 */    57,  265,  127,  265,  128,  132,  155,  102,   62,   77,
 /*   430 */    78,   95,   96,   97,   83,   65,   72,   87,   71,  138,
 /*   440 */   106,  121,   57,  265,  127,  265,  128,  132,  156,  102,
 /*   450 */    62,   77,   78,   95,   96,   97,   83,   65,   72,   87,
 /*   460 */    71,  138,  106,  121,   57,  265,  127,  265,  128,  132,
 /*   470 */   157,  102,   62,   77,   78,   95,   96,   97,   83,   65,
 /*   480 */    72,   87,   71,  138,  106,  121,   57,  265,  127,  265,
 /*   490 */   128,  132,  158,  102,   62,   77,   78,   95,   96,   97,
 /*   500 */    83,   65,   72,   87,   71,  138,  106,  121,   57,  265,
 /*   510 */   127,  265,  128,  132,  159,  102,   62,   77,   78,   95,
 /*   520 */    96,   97,   83,   65,   72,   87,   71,  138,  106,  121,
 /*   530 */    57,  265,  127,  265,  128,  132,  160,  102,   62,   77,
 /*   540 */    78,   95,   96,   97,   83,   65,   72,   87,   71,  138,
 /*   550 */   106,  121,   57,  265,  127,  265,  128,  132,  161,  102,
 /*   560 */    62,   77,   78,   95,   96,   97,   83,   65,   72,   87,
 /*   570 */    71,  138,  106,  121,   57,  265,  127,  265,  128,  132,
 /*   580 */   162,  102,   62,   77,   78,   95,   96,   97,   83,   65,
 /*   590 */    72,   87,   71,  138,  106,  121,   57,  265,  127,  265,
 /*   600 */   128,  132,  163,  102,   62,   77,   78,   95,   96,   97,
 /*   610 */    83,   65,   72,   87,   71,  138,  106,  121,   57,  265,
 /*   620 */   127,  265,  128,  132,  164,  102,   62,   77,   78,   95,
 /*   630 */    96,   97,   83,   65,   72,   87,   71,  138,  106,  121,
 /*   640 */    57,  265,  127,  265,  128,  132,   81,  265,   90,   95,
 /*   650 */    96,   97,   83,   65,   72,   87,   71,  138,  106,  121,
 /*   660 */    57,  265,  127,   81,  128,  132,   92,   96,   97,   83,
 /*   670 */    65,   72,   87,   71,  138,  106,  121,   57,  265,  127,
 /*   680 */    81,  128,  132,  100,   96,   97,   83,   65,   72,   87,
 /*   690 */    71,  138,  106,  121,   57,  265,  127,   81,  128,  132,
 /*   700 */     6,   93,   97,   83,   65,   72,   87,   71,  138,  106,
 /*   710 */   121,   57,   81,  127,   58,  128,  132,   94,   83,   65,
 /*   720 */    72,   87,   71,  138,  106,  121,   57,  265,  127,  265,
 /*   730 */   128,  132,  265,    7,   12,   13,   14,   15,   16,   17,
 /*   740 */    18,   19,   20,   21,   22,  265,   81,  265,  129,  265,
 /*   750 */   265,  265,   79,   65,   72,   87,   71,  138,  106,  121,
 /*   760 */    57,  265,  127,  265,  128,  132,   81,   11,  265,  265,
 /*   770 */   119,  120,  142,   63,   72,   87,   71,  138,  106,  121,
 /*   780 */    57,  265,  127,   81,  128,  132,  265,  265,  265,  265,
 /*   790 */    64,   72,   87,   71,  138,  106,  121,   57,   81,  127,
 /*   800 */   265,  128,  132,  265,  265,  265,   66,   87,   71,  138,
 /*   810 */   106,  121,   57,   81,  127,  152,  128,  132,  265,  265,
 /*   820 */   265,   68,   87,   71,  138,  106,  121,   57,   61,  127,
 /*   830 */   265,  128,  132,   81,  265,  265,    2,  265,  265,  265,
 /*   840 */    99,   70,   87,   71,  138,  106,  121,   57,  265,  127,
 /*   850 */    81,  128,  132,  265,  265,  265,  265,  265,   73,   87,
 /*   860 */    71,  138,  106,  121,   57,   81,  127,  265,  128,  132,
 /*   870 */   265,  265,  265,   74,   87,   71,  138,  106,  121,   57,
 /*   880 */    81,  127,  265,  128,  132,  265,  265,  265,   75,   87,
 /*   890 */    71,  138,  106,  121,   57,  265,  127,  265,  128,  132,
 /*   900 */    81,  265,  265,  265,  265,  265,  265,  265,  265,   80,
 /*   910 */    71,  138,  106,  121,   57,  265,  127,   81,  128,  132,
 /*   920 */   265,  265,  265,  265,  265,  265,   84,   71,  138,  106,
 /*   930 */   121,   57,   81,  127,  265,  128,  132,  265,  265,  265,
 /*   940 */   265,   86,   71,  138,  106,  121,   57,   81,  127,  265,
 /*   950 */   128,  132,  265,  265,  265,  265,  265,   67,  138,  106,
 /*   960 */   121,   57,  265,  127,  265,  128,  132,   81,  265,  265,
 /*   970 */   265,  265,  265,  265,  265,  265,   81,   69,  138,  106,
 /*   980 */   121,   57,  265,  127,  265,  128,  132,  105,  106,  121,
 /*   990 */    57,  265,  127,   81,  128,  132,  265,  265,  265,  265,
 /*  1000 */   265,  265,   81,  265,  107,  106,  121,   57,  265,  127,
 /*  1010 */   265,  128,  132,  108,  106,  121,   57,   81,  127,  265,
 /*  1020 */   128,  132,  265,  265,  265,  265,   81,  265,  109,  106,
 /*  1030 */   121,   57,  265,  127,  265,  128,  132,  110,  106,  121,
 /*  1040 */    57,  265,  127,   81,  128,  132,  265,  265,  265,  265,
 /*  1050 */   265,  265,   81,  265,  111,  106,  121,   57,  265,  127,
 /*  1060 */   265,  128,  132,  112,  106,  121,   57,  265,  127,   81,
 /*  1070 */   128,  132,  265,  265,  265,  265,  265,  265,   81,  265,
 /*  1080 */   113,  106,  121,   57,  265,  127,  265,  128,  132,  114,
 /*  1090 */   106,  121,   57,   81,  127,  265,  128,  132,  265,  265,
 /*  1100 */   265,  265,   81,  265,  115,  106,  121,   57,  265,  127,
 /*  1110 */   265,  128,  132,  116,  106,  121,   57,  265,  127,   81,
 /*  1120 */   128,  132,  265,  265,  265,  265,  265,  265,   81,  265,
 /*  1130 */   117,  106,  121,   57,  265,  127,  265,  128,  132,  118,
 /*  1140 */   106,  121,   57,  265,  127,   81,  128,  132,  265,  265,
 /*  1150 */   265,  265,  265,  265,   81,  265,  126,  106,  121,   57,
 /*  1160 */   265,  127,  265,  128,  132,  130,  106,  121,   57,  265,
 /*  1170 */   127,  265,  128,  132,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     1,   62,   63,   64,   65,   66,   67,   68,   69,   70,
 /*    10 */    71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*    20 */    14,   82,   16,   84,   85,   24,   25,   26,   27,   28,
 /*    30 */    29,   86,   33,   34,    1,   89,   90,   38,   39,   40,
 /*    40 */    41,   42,   43,   44,   45,   46,   47,   48,    1,   50,
 /*    50 */    51,   52,   53,   54,   55,   56,   57,   58,   50,   51,
 /*    60 */     1,   53,   35,   36,   37,   88,   33,   34,    1,    0,
 /*    70 */     1,   38,   39,   40,   41,   42,   43,   44,   45,   46,
 /*    80 */    47,   48,   91,   50,   51,   52,   53,   54,   55,   56,
 /*    90 */    57,   58,   30,   31,   32,   81,   19,   83,   17,   18,
 /*   100 */    33,   34,   22,   23,   57,   38,   39,   40,   41,   42,
 /*   110 */    43,   44,   45,   46,   47,   48,   57,   50,   51,   52,
 /*   120 */    53,   54,   55,   56,   15,   58,   33,   34,   39,   40,
 /*   130 */    20,   15,   64,   65,   66,   67,   68,   69,   70,   71,
 /*   140 */    72,   73,   74,   75,   76,   77,   78,   79,   80,   21,
 /*   150 */    82,   50,   84,   85,   86,   87,   33,   34,   92,   92,
 /*   160 */    92,   38,   39,   40,   41,   42,   43,   44,   45,   46,
 /*   170 */    47,   48,   92,   50,   51,   52,   53,   54,   55,   56,
 /*   180 */    92,   58,   63,   64,   65,   66,   67,   68,   69,   70,
 /*   190 */    71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*   200 */    92,   82,   92,   84,   85,   63,   64,   65,   66,   67,
 /*   210 */    68,   69,   70,   71,   72,   73,   74,   75,   76,   77,
 /*   220 */    78,   79,   80,   92,   82,   92,   84,   85,   64,   65,
 /*   230 */    66,   67,   68,   69,   70,   71,   72,   73,   74,   75,
 /*   240 */    76,   77,   78,   79,   80,   92,   82,   92,   84,   85,
 /*   250 */    64,   65,   66,   67,   68,   69,   70,   71,   72,   73,
 /*   260 */    74,   75,   76,   77,   78,   79,   80,   92,   82,   92,
 /*   270 */    84,   85,   64,   65,   66,   67,   68,   69,   70,   71,
 /*   280 */    72,   73,   74,   75,   76,   77,   78,   79,   80,   92,
 /*   290 */    82,   92,   84,   85,   64,   65,   66,   67,   68,   69,
 /*   300 */    70,   71,   72,   73,   74,   75,   76,   77,   78,   79,
 /*   310 */    80,   92,   82,   92,   84,   85,   64,   65,   66,   67,
 /*   320 */    68,   69,   70,   71,   72,   73,   74,   75,   76,   77,
 /*   330 */    78,   79,   80,   92,   82,   92,   84,   85,   64,   65,
 /*   340 */    66,   67,   68,   69,   70,   71,   72,   73,   74,   75,
 /*   350 */    76,   77,   78,   79,   80,   92,   82,   92,   84,   85,
 /*   360 */    64,   65,   66,   67,   68,   69,   70,   71,   72,   73,
 /*   370 */    74,   75,   76,   77,   78,   79,   80,   92,   82,   92,
 /*   380 */    84,   85,   64,   65,   66,   67,   68,   69,   70,   71,
 /*   390 */    72,   73,   74,   75,   76,   77,   78,   79,   80,   92,
 /*   400 */    82,   92,   84,   85,   64,   65,   66,   67,   68,   69,
 /*   410 */    70,   71,   72,   73,   74,   75,   76,   77,   78,   79,
 /*   420 */    80,   92,   82,   92,   84,   85,   64,   65,   66,   67,
 /*   430 */    68,   69,   70,   71,   72,   73,   74,   75,   76,   77,
 /*   440 */    78,   79,   80,   92,   82,   92,   84,   85,   64,   65,
 /*   450 */    66,   67,   68,   69,   70,   71,   72,   73,   74,   75,
 /*   460 */    76,   77,   78,   79,   80,   92,   82,   92,   84,   85,
 /*   470 */    64,   65,   66,   67,   68,   69,   70,   71,   72,   73,
 /*   480 */    74,   75,   76,   77,   78,   79,   80,   92,   82,   92,
 /*   490 */    84,   85,   64,   65,   66,   67,   68,   69,   70,   71,
 /*   500 */    72,   73,   74,   75,   76,   77,   78,   79,   80,   92,
 /*   510 */    82,   92,   84,   85,   64,   65,   66,   67,   68,   69,
 /*   520 */    70,   71,   72,   73,   74,   75,   76,   77,   78,   79,
 /*   530 */    80,   92,   82,   92,   84,   85,   64,   65,   66,   67,
 /*   540 */    68,   69,   70,   71,   72,   73,   74,   75,   76,   77,
 /*   550 */    78,   79,   80,   92,   82,   92,   84,   85,   64,   65,
 /*   560 */    66,   67,   68,   69,   70,   71,   72,   73,   74,   75,
 /*   570 */    76,   77,   78,   79,   80,   92,   82,   92,   84,   85,
 /*   580 */    64,   65,   66,   67,   68,   69,   70,   71,   72,   73,
 /*   590 */    74,   75,   76,   77,   78,   79,   80,   92,   82,   92,
 /*   600 */    84,   85,   64,   65,   66,   67,   68,   69,   70,   71,
 /*   610 */    72,   73,   74,   75,   76,   77,   78,   79,   80,   92,
 /*   620 */    82,   92,   84,   85,   64,   65,   66,   67,   68,   69,
 /*   630 */    70,   71,   72,   73,   74,   75,   76,   77,   78,   79,
 /*   640 */    80,   92,   82,   92,   84,   85,   66,   92,   68,   69,
 /*   650 */    70,   71,   72,   73,   74,   75,   76,   77,   78,   79,
 /*   660 */    80,   92,   82,   66,   84,   85,   69,   70,   71,   72,
 /*   670 */    73,   74,   75,   76,   77,   78,   79,   80,   92,   82,
 /*   680 */    66,   84,   85,   69,   70,   71,   72,   73,   74,   75,
 /*   690 */    76,   77,   78,   79,   80,   92,   82,   66,   84,   85,
 /*   700 */     1,   70,   71,   72,   73,   74,   75,   76,   77,   78,
 /*   710 */    79,   80,   66,   82,    1,   84,   85,   71,   72,   73,
 /*   720 */    74,   75,   76,   77,   78,   79,   80,   92,   82,   92,
 /*   730 */    84,   85,   92,    2,    3,    4,    5,    6,    7,    8,
 /*   740 */     9,   10,   11,   12,   13,   92,   66,   92,   49,   92,
 /*   750 */    92,   92,   72,   73,   74,   75,   76,   77,   78,   79,
 /*   760 */    80,   92,   82,   92,   84,   85,   66,    1,   92,   92,
 /*   770 */    39,   40,   59,   73,   74,   75,   76,   77,   78,   79,
 /*   780 */    80,   92,   82,   66,   84,   85,   92,   92,   92,   92,
 /*   790 */    73,   74,   75,   76,   77,   78,   79,   80,   66,   82,
 /*   800 */    92,   84,   85,   92,   92,   92,   74,   75,   76,   77,
 /*   810 */    78,   79,   80,   66,   82,   49,   84,   85,   92,   92,
 /*   820 */    92,   74,   75,   76,   77,   78,   79,   80,   48,   82,
 /*   830 */    92,   84,   85,   66,   92,   92,   56,   92,   92,   92,
 /*   840 */    60,   74,   75,   76,   77,   78,   79,   80,   92,   82,
 /*   850 */    66,   84,   85,   92,   92,   92,   92,   92,   74,   75,
 /*   860 */    76,   77,   78,   79,   80,   66,   82,   92,   84,   85,
 /*   870 */    92,   92,   92,   74,   75,   76,   77,   78,   79,   80,
 /*   880 */    66,   82,   92,   84,   85,   92,   92,   92,   74,   75,
 /*   890 */    76,   77,   78,   79,   80,   92,   82,   92,   84,   85,
 /*   900 */    66,   92,   92,   92,   92,   92,   92,   92,   92,   75,
 /*   910 */    76,   77,   78,   79,   80,   92,   82,   66,   84,   85,
 /*   920 */    92,   92,   92,   92,   92,   92,   75,   76,   77,   78,
 /*   930 */    79,   80,   66,   82,   92,   84,   85,   92,   92,   92,
 /*   940 */    92,   75,   76,   77,   78,   79,   80,   66,   82,   92,
 /*   950 */    84,   85,   92,   92,   92,   92,   92,   76,   77,   78,
 /*   960 */    79,   80,   92,   82,   92,   84,   85,   66,   92,   92,
 /*   970 */    92,   92,   92,   92,   92,   92,   66,   76,   77,   78,
 /*   980 */    79,   80,   92,   82,   92,   84,   85,   77,   78,   79,
 /*   990 */    80,   92,   82,   66,   84,   85,   92,   92,   92,   92,
 /*  1000 */    92,   92,   66,   92,   77,   78,   79,   80,   92,   82,
 /*  1010 */    92,   84,   85,   77,   78,   79,   80,   66,   82,   92,
 /*  1020 */    84,   85,   92,   92,   92,   92,   66,   92,   77,   78,
 /*  1030 */    79,   80,   92,   82,   92,   84,   85,   77,   78,   79,
 /*  1040 */    80,   92,   82,   66,   84,   85,   92,   92,   92,   92,
 /*  1050 */    92,   92,   66,   92,   77,   78,   79,   80,   92,   82,
 /*  1060 */    92,   84,   85,   77,   78,   79,   80,   92,   82,   66,
 /*  1070 */    84,   85,   92,   92,   92,   92,   92,   92,   66,   92,
 /*  1080 */    77,   78,   79,   80,   92,   82,   92,   84,   85,   77,
 /*  1090 */    78,   79,   80,   66,   82,   92,   84,   85,   92,   92,
 /*  1100 */    92,   92,   66,   92,   77,   78,   79,   80,   92,   82,
 /*  1110 */    92,   84,   85,   77,   78,   79,   80,   92,   82,   66,
 /*  1120 */    84,   85,   92,   92,   92,   92,   92,   92,   66,   92,
 /*  1130 */    77,   78,   79,   80,   92,   82,   92,   84,   85,   77,
 /*  1140 */    78,   79,   80,   92,   82,   66,   84,   85,   92,   92,
 /*  1150 */    92,   92,   92,   92,   66,   92,   77,   78,   79,   80,
 /*  1160 */    92,   82,   92,   84,   85,   77,   78,   79,   80,   92,
 /*  1170 */    82,   92,   84,   85,
};
#define YY_SHIFT_USE_DFLT (-2)
#define YY_SHIFT_MAX 100
static const short yy_shift_ofst[] = {
 /*     0 */   123,   67,  123,  123,   -1,   33,  123,  123,  123,  123,
 /*    10 */   123,  123,  123,  123,  123,  123,  123,  123,  123,  123,
 /*    20 */   123,  123,  123,  123,  123,  123,  123,  123,  123,  123,
 /*    30 */   123,  123,  123,  123,  123,  123,  123,  123,  123,  123,
 /*    40 */   123,  123,  123,  123,  123,  123,  123,  123,  123,  123,
 /*    50 */   123,  123,  123,  123,  123,  123,  123,  780,    8,   47,
 /*    60 */    -2,   -2,  731,    1,    1,    1,   62,   27,   62,   27,
 /*    70 */    62,   27,   62,   62,   62,   62,   69,    6,   81,   80,
 /*    80 */    93,   89,   59,   80,   93,  699,   93,   93,  713,  766,
 /*    90 */    81,  109,   77,  110,  128,   77,  110,  128,  116,  101,
 /*   100 */    77,
};
#define YY_REDUCE_USE_DFLT (-62)
#define YY_REDUCE_MAX 61
static const short yy_reduce_ofst[] = {
 /*     0 */   -61,   68,  119,  142,  164,  186,  208,  230,  252,  274,
 /*    10 */   296,  318,  340,  362,  384,  406,  428,  450,  472,  494,
 /*    20 */   516,  538,  560,  580,  597,  614,  631,  646,  680,  700,
 /*    30 */   717,  732,  747,  767,  784,  799,  814,  834,  851,  866,
 /*    40 */   881,  901,  910,  927,  936,  951,  960,  977,  986, 1003,
 /*    50 */  1012, 1027, 1036, 1053, 1062, 1079, 1088,   14,  -54,  -55,
 /*    60 */   -23,   -9,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   263,  263,  263,  263,  263,  263,  263,  263,  263,  263,
 /*    10 */   263,  263,  263,  263,  263,  263,  263,  263,  263,  263,
 /*    20 */   263,  263,  263,  263,  263,  263,  263,  263,  263,  263,
 /*    30 */   263,  263,  263,  263,  263,  263,  263,  263,  263,  263,
 /*    40 */   263,  263,  263,  263,  263,  263,  263,  263,  263,  263,
 /*    50 */   263,  263,  263,  263,  263,  263,  263,  232,  263,  263,
 /*    60 */   254,  261,  228,  195,  196,  194,  198,  209,  199,  210,
 /*    70 */   200,  208,  197,  201,  202,  203,  263,  181,  183,  193,
 /*    80 */   205,  228,  263,  192,  206,  263,  207,  204,  263,  263,
 /*    90 */   184,  263,  186,  189,  191,  185,  188,  190,  263,  263,
 /*   100 */   187,  167,  168,  169,  182,  212,  215,  216,  217,  218,
 /*   110 */   219,  220,  221,  222,  223,  224,  225,  226,  227,  229,
 /*   120 */   230,  231,  233,  235,  166,  258,  213,  234,  236,  237,
 /*   130 */   214,  238,  239,  240,  241,  242,  243,  244,  211,  245,
 /*   140 */   249,  251,  253,  255,  256,  257,  246,  252,  247,  248,
 /*   150 */   250,  259,  260,  262,  170,  171,  172,  173,  174,  175,
 /*   160 */   176,  177,  178,  179,  180,
};
#define YY_SZ_ACTTAB (int)(sizeof(yy_action)/sizeof(yy_action[0]))

/* The next table maps tokens into fallback tokens.  If a construct
** like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammer, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
*/
struct yyStackEntry {
  int stateno;       /* The state-number */
  int major;         /* The major token value.  This is the code
                     ** number for the token at this stack level */
  YYMINORTYPE minor; /* The user-supplied minor token value.  This
                     ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
  int yyerrcnt;                 /* Shifts left before out of the error */
  grn_expr_parserARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void grn_expr_parserTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "COMMA",         "ASSIGN",        "STAR_ASSIGN", 
  "SLASH_ASSIGN",  "MOD_ASSIGN",    "PLUS_ASSIGN",   "MINUS_ASSIGN",
  "SHIFTL_ASSIGN",  "SHIRTR_ASSIGN",  "SHIFTRR_ASSIGN",  "AND_ASSIGN",  
  "XOR_ASSIGN",    "OR_ASSIGN",     "QUESTION",      "COLON",       
  "LOGICAL_OR",    "LOGICAL_AND",   "LOGICAL_BUT",   "BITWISE_OR",  
  "BITWISE_XOR",   "BITWISE_AND",   "EQUAL",         "NOT_EQUAL",   
  "LESS",          "GREATER",       "LESS_EQUAL",    "GREATER_EQUAL",
  "IN",            "MATCH",         "SHIFTL",        "SHIFTR",      
  "SHIFTRR",       "PLUS",          "MINUS",         "STAR",        
  "SLASH",         "MOD",           "DELETE",        "INCR",        
  "DECR",          "NOT",           "ADJ_INC",       "ADJ_DEC",     
  "ADJ_NEG",       "UNARY_SIMILAR",  "UNARY_EXTRACT",  "UNARY_NEAR",  
  "PARENL",        "PARENR",        "IDENTIFIER",    "DECIMAL",     
  "HEX_INTEGER",   "STRING",        "BOOLEAN",       "NULL",        
  "BRACKETL",      "BRACKETR",      "BRACEL",        "RBRACE",      
  "DOT",           "error",         "query",         "expression",  
  "assignment_expression",  "conditional_expression",  "lefthand_side_expression",  "logical_or_expression",
  "logical_and_expression",  "bitwise_or_expression",  "bitwise_xor_expression",  "bitwise_and_expression",
  "equality_expression",  "relational_expression",  "shift_expression",  "additive_expression",
  "multiplicative_expression",  "unary_expression",  "postfix_expression",  "call_expression",
  "member_expression",  "arguments",     "primary_expression",  "member_expression_part",
  "object_literal",  "array_literal",  "elision",       "element_list",
  "property_name_and_value_list",  "property_name_and_value",  "property_name",  "argument_list",
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "query ::= expression",
 /*   1 */ "expression ::= assignment_expression",
 /*   2 */ "expression ::= expression COMMA assignment_expression",
 /*   3 */ "assignment_expression ::= conditional_expression",
 /*   4 */ "assignment_expression ::= lefthand_side_expression ASSIGN assignment_expression",
 /*   5 */ "assignment_expression ::= lefthand_side_expression STAR_ASSIGN assignment_expression",
 /*   6 */ "assignment_expression ::= lefthand_side_expression SLASH_ASSIGN assignment_expression",
 /*   7 */ "assignment_expression ::= lefthand_side_expression MOD_ASSIGN assignment_expression",
 /*   8 */ "assignment_expression ::= lefthand_side_expression PLUS_ASSIGN assignment_expression",
 /*   9 */ "assignment_expression ::= lefthand_side_expression MINUS_ASSIGN assignment_expression",
 /*  10 */ "assignment_expression ::= lefthand_side_expression SHIFTL_ASSIGN assignment_expression",
 /*  11 */ "assignment_expression ::= lefthand_side_expression SHIRTR_ASSIGN assignment_expression",
 /*  12 */ "assignment_expression ::= lefthand_side_expression SHIFTRR_ASSIGN assignment_expression",
 /*  13 */ "assignment_expression ::= lefthand_side_expression AND_ASSIGN assignment_expression",
 /*  14 */ "assignment_expression ::= lefthand_side_expression XOR_ASSIGN assignment_expression",
 /*  15 */ "assignment_expression ::= lefthand_side_expression OR_ASSIGN assignment_expression",
 /*  16 */ "conditional_expression ::= logical_or_expression",
 /*  17 */ "conditional_expression ::= logical_or_expression QUESTION assignment_expression COLON assignment_expression",
 /*  18 */ "logical_or_expression ::= logical_and_expression",
 /*  19 */ "logical_or_expression ::= logical_or_expression LOGICAL_OR logical_and_expression",
 /*  20 */ "logical_and_expression ::= bitwise_or_expression",
 /*  21 */ "logical_and_expression ::= logical_and_expression LOGICAL_AND bitwise_or_expression",
 /*  22 */ "logical_and_expression ::= logical_and_expression LOGICAL_BUT bitwise_or_expression",
 /*  23 */ "bitwise_or_expression ::= bitwise_xor_expression",
 /*  24 */ "bitwise_or_expression ::= bitwise_or_expression BITWISE_OR bitwise_xor_expression",
 /*  25 */ "bitwise_xor_expression ::= bitwise_and_expression",
 /*  26 */ "bitwise_xor_expression ::= bitwise_xor_expression BITWISE_XOR bitwise_and_expression",
 /*  27 */ "bitwise_and_expression ::= equality_expression",
 /*  28 */ "bitwise_and_expression ::= bitwise_and_expression BITWISE_AND equality_expression",
 /*  29 */ "equality_expression ::= relational_expression",
 /*  30 */ "equality_expression ::= equality_expression EQUAL relational_expression",
 /*  31 */ "equality_expression ::= equality_expression NOT_EQUAL relational_expression",
 /*  32 */ "relational_expression ::= shift_expression",
 /*  33 */ "relational_expression ::= relational_expression LESS shift_expression",
 /*  34 */ "relational_expression ::= relational_expression GREATER shift_expression",
 /*  35 */ "relational_expression ::= relational_expression LESS_EQUAL shift_expression",
 /*  36 */ "relational_expression ::= relational_expression GREATER_EQUAL shift_expression",
 /*  37 */ "relational_expression ::= relational_expression IN shift_expression",
 /*  38 */ "relational_expression ::= relational_expression MATCH shift_expression",
 /*  39 */ "shift_expression ::= additive_expression",
 /*  40 */ "shift_expression ::= shift_expression SHIFTL additive_expression",
 /*  41 */ "shift_expression ::= shift_expression SHIFTR additive_expression",
 /*  42 */ "shift_expression ::= shift_expression SHIFTRR additive_expression",
 /*  43 */ "additive_expression ::= multiplicative_expression",
 /*  44 */ "additive_expression ::= additive_expression PLUS multiplicative_expression",
 /*  45 */ "additive_expression ::= additive_expression MINUS multiplicative_expression",
 /*  46 */ "multiplicative_expression ::= unary_expression",
 /*  47 */ "multiplicative_expression ::= multiplicative_expression STAR unary_expression",
 /*  48 */ "multiplicative_expression ::= multiplicative_expression SLASH unary_expression",
 /*  49 */ "multiplicative_expression ::= multiplicative_expression MOD unary_expression",
 /*  50 */ "unary_expression ::= postfix_expression",
 /*  51 */ "unary_expression ::= DELETE unary_expression",
 /*  52 */ "unary_expression ::= INCR unary_expression",
 /*  53 */ "unary_expression ::= DECR unary_expression",
 /*  54 */ "unary_expression ::= PLUS unary_expression",
 /*  55 */ "unary_expression ::= MINUS unary_expression",
 /*  56 */ "unary_expression ::= NOT unary_expression",
 /*  57 */ "unary_expression ::= ADJ_INC unary_expression",
 /*  58 */ "unary_expression ::= ADJ_DEC unary_expression",
 /*  59 */ "unary_expression ::= ADJ_NEG unary_expression",
 /*  60 */ "unary_expression ::= UNARY_SIMILAR unary_expression",
 /*  61 */ "unary_expression ::= UNARY_EXTRACT unary_expression",
 /*  62 */ "unary_expression ::= UNARY_NEAR unary_expression",
 /*  63 */ "postfix_expression ::= lefthand_side_expression",
 /*  64 */ "postfix_expression ::= lefthand_side_expression INCR",
 /*  65 */ "postfix_expression ::= lefthand_side_expression DECR",
 /*  66 */ "lefthand_side_expression ::= call_expression",
 /*  67 */ "lefthand_side_expression ::= member_expression",
 /*  68 */ "call_expression ::= member_expression arguments",
 /*  69 */ "member_expression ::= primary_expression",
 /*  70 */ "member_expression ::= member_expression member_expression_part",
 /*  71 */ "primary_expression ::= object_literal",
 /*  72 */ "primary_expression ::= PARENL expression PARENR",
 /*  73 */ "primary_expression ::= IDENTIFIER",
 /*  74 */ "primary_expression ::= array_literal",
 /*  75 */ "primary_expression ::= DECIMAL",
 /*  76 */ "primary_expression ::= HEX_INTEGER",
 /*  77 */ "primary_expression ::= STRING",
 /*  78 */ "primary_expression ::= BOOLEAN",
 /*  79 */ "primary_expression ::= NULL",
 /*  80 */ "array_literal ::= BRACKETL elision BRACKETR",
 /*  81 */ "array_literal ::= BRACKETL element_list elision BRACKETR",
 /*  82 */ "array_literal ::= BRACKETL element_list BRACKETR",
 /*  83 */ "elision ::= COMMA",
 /*  84 */ "elision ::= elision COMMA",
 /*  85 */ "element_list ::= assignment_expression",
 /*  86 */ "element_list ::= elision assignment_expression",
 /*  87 */ "element_list ::= element_list elision assignment_expression",
 /*  88 */ "object_literal ::= BRACEL property_name_and_value_list RBRACE",
 /*  89 */ "property_name_and_value_list ::=",
 /*  90 */ "property_name_and_value_list ::= property_name_and_value_list COMMA property_name_and_value",
 /*  91 */ "property_name_and_value ::= property_name COLON assignment_expression",
 /*  92 */ "property_name ::= IDENTIFIER|STRING|DECIMAL",
 /*  93 */ "member_expression_part ::= BRACKETL expression BRACKETR",
 /*  94 */ "member_expression_part ::= DOT IDENTIFIER",
 /*  95 */ "arguments ::= PARENL argument_list PARENR",
 /*  96 */ "argument_list ::=",
 /*  97 */ "argument_list ::= argument_list COMMA assignment_expression",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(yyParser *p){
  int newSize;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  if( pNew ){
    p->yystack = pNew;
    p->yystksz = newSize;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, p->yystksz);
    }
#endif
  }
}
#endif

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to grn_expr_parser and grn_expr_parserFree.
*/
void *grn_expr_parserAlloc(void *(*mallocProc)(size_t)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (size_t)sizeof(yyParser) );
  if( pParser ){
    pParser->yyidx = -1;
#if YYSTACKDEPTH<=0
    yyGrowStack(pParser);
#endif
  }
  return pParser;
}

/* The following function deletes the value associated with a
** symbol.  The symbol can be either a terminal or nonterminal.
** "yymajor" is the symbol code, and "yypminor" is a pointer to
** the value.
*/
static void yy_destructor(YYCODETYPE yymajor, YYMINORTYPE *yypminor){
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are not used
    ** inside the C code.
    */
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
**
** Return the major token number for the symbol popped.
*/
static int yy_pop_parser_stack(yyParser *pParser){
  YYCODETYPE yymajor;
  yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];

  if( pParser->yyidx<0 ) return 0;
#ifndef NDEBUG
  if( yyTraceFILE && pParser->yyidx>=0 ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yymajor = yytos->major;
  yy_destructor( yymajor, &yytos->minor);
  pParser->yyidx--;
  return yymajor;
}

/* 
** Deallocate and destroy a parser.  Destructors are all called for
** all stack elements before shutting the parser down.
**
** Inputs:
** <ul>
** <li>  A pointer to the parser.  This should be a pointer
**       obtained from grn_expr_parserAlloc.
** <li>  A pointer to a function used to reclaim memory obtained
**       from malloc.
** </ul>
*/
void grn_expr_parserFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
  if( pParser==0 ) return;
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;
 
  if( stateno>YY_SHIFT_MAX || (i = yy_shift_ofst[stateno])==YY_SHIFT_USE_DFLT ){
    return yy_default[stateno];
  }
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    if( iLookAhead>0 ){
#ifdef YYFALLBACK
      int iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        return yy_find_shift_action(pParser, iFallback);
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( j>=0 && j<YY_SZ_ACTTAB && yy_lookahead[j]==YYWILDCARD ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
    }
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_MAX ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_MAX );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_SZ_ACTTAB );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser, YYMINORTYPE *yypMinor){
   grn_expr_parserARG_FETCH;
   yypParser->yyidx--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
   grn_expr_parserARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer ot the minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
#if YYSTACKDEPTH>0 
  if( yypParser->yyidx>=YYSTACKDEPTH ){
    yyStackOverflow(yypParser, yypMinor);
    return;
  }
#else
  if( yypParser->yyidx>=yypParser->yystksz ){
    yyGrowStack(yypParser);
    if( yypParser->yyidx>=yypParser->yystksz ){
      yyStackOverflow(yypParser, yypMinor);
      return;
    }
  }
#endif
  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->stateno = yyNewState;
  yytos->major = yyMajor;
  yytos->minor = *yypMinor;
#ifndef NDEBUG
  if( yyTraceFILE && yypParser->yyidx>0 ){
    int i;
    fprintf(yyTraceFILE,"%sShift %d\n",yyTracePrompt,yyNewState);
    fprintf(yyTraceFILE,"%sStack:",yyTracePrompt);
    for(i=1; i<=yypParser->yyidx; i++)
      fprintf(yyTraceFILE," %s",yyTokenName[yypParser->yystack[i].major]);
    fprintf(yyTraceFILE,"\n");
  }
#endif
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 62, 1 },
  { 63, 1 },
  { 63, 3 },
  { 64, 1 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 64, 3 },
  { 65, 1 },
  { 65, 5 },
  { 67, 1 },
  { 67, 3 },
  { 68, 1 },
  { 68, 3 },
  { 68, 3 },
  { 69, 1 },
  { 69, 3 },
  { 70, 1 },
  { 70, 3 },
  { 71, 1 },
  { 71, 3 },
  { 72, 1 },
  { 72, 3 },
  { 72, 3 },
  { 73, 1 },
  { 73, 3 },
  { 73, 3 },
  { 73, 3 },
  { 73, 3 },
  { 73, 3 },
  { 73, 3 },
  { 74, 1 },
  { 74, 3 },
  { 74, 3 },
  { 74, 3 },
  { 75, 1 },
  { 75, 3 },
  { 75, 3 },
  { 76, 1 },
  { 76, 3 },
  { 76, 3 },
  { 76, 3 },
  { 77, 1 },
  { 77, 2 },
  { 77, 2 },
  { 77, 2 },
  { 77, 2 },
  { 77, 2 },
  { 77, 2 },
  { 77, 2 },
  { 77, 2 },
  { 77, 2 },
  { 77, 2 },
  { 77, 2 },
  { 77, 2 },
  { 78, 1 },
  { 78, 2 },
  { 78, 2 },
  { 66, 1 },
  { 66, 1 },
  { 79, 2 },
  { 80, 1 },
  { 80, 2 },
  { 82, 1 },
  { 82, 3 },
  { 82, 1 },
  { 82, 1 },
  { 82, 1 },
  { 82, 1 },
  { 82, 1 },
  { 82, 1 },
  { 82, 1 },
  { 85, 3 },
  { 85, 4 },
  { 85, 3 },
  { 86, 1 },
  { 86, 2 },
  { 87, 1 },
  { 87, 2 },
  { 87, 3 },
  { 84, 3 },
  { 88, 0 },
  { 88, 3 },
  { 89, 3 },
  { 90, 1 },
  { 83, 3 },
  { 83, 2 },
  { 81, 3 },
  { 91, 0 },
  { 91, 3 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  int yyruleno                 /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  grn_expr_parserARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno>=0 
        && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    fprintf(yyTraceFILE, "%sReduce [%s].\n", yyTracePrompt,
      yyRuleName[yyruleno]);
  }
#endif /* NDEBUG */

  /* Silence complaints from purify about yygotominor being uninitialized
  ** in some cases when it is copied into the stack after the following
  ** switch.  yygotominor is uninitialized when a rule reduces that does
  ** not set the value of its left-hand side nonterminal.  Leaving the
  ** value of the nonterminal uninitialized is utterly harmless as long
  ** as the value is never used.  So really the only thing this code
  ** accomplishes is to quieten purify.  
  **
  ** 2007-01-16:  The wireshark project (www.wireshark.org) reports that
  ** without this code, their parser segfaults.  I'm not sure what there
  ** parser is doing to make this happen.  This is the second bug report
  ** from wireshark this week.  Clearly they are stressing Lemon in ways
  ** that it has not been previously stressed...  (SQLite ticket #2172)
  */
  /*memset(&yygotominor, 0, sizeof(yygotominor));*/
  yygotominor = yyzerominor;


  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
      case 0: /* query ::= expression */
      case 1: /* expression ::= assignment_expression */
      case 2: /* expression ::= expression COMMA assignment_expression */
      case 3: /* assignment_expression ::= conditional_expression */
      case 4: /* assignment_expression ::= lefthand_side_expression ASSIGN assignment_expression */
      case 5: /* assignment_expression ::= lefthand_side_expression STAR_ASSIGN assignment_expression */
      case 6: /* assignment_expression ::= lefthand_side_expression SLASH_ASSIGN assignment_expression */
      case 7: /* assignment_expression ::= lefthand_side_expression MOD_ASSIGN assignment_expression */
      case 8: /* assignment_expression ::= lefthand_side_expression PLUS_ASSIGN assignment_expression */
      case 9: /* assignment_expression ::= lefthand_side_expression MINUS_ASSIGN assignment_expression */
      case 10: /* assignment_expression ::= lefthand_side_expression SHIFTL_ASSIGN assignment_expression */
      case 11: /* assignment_expression ::= lefthand_side_expression SHIRTR_ASSIGN assignment_expression */
      case 12: /* assignment_expression ::= lefthand_side_expression SHIFTRR_ASSIGN assignment_expression */
      case 13: /* assignment_expression ::= lefthand_side_expression AND_ASSIGN assignment_expression */
      case 14: /* assignment_expression ::= lefthand_side_expression XOR_ASSIGN assignment_expression */
      case 15: /* assignment_expression ::= lefthand_side_expression OR_ASSIGN assignment_expression */
      case 16: /* conditional_expression ::= logical_or_expression */
      case 17: /* conditional_expression ::= logical_or_expression QUESTION assignment_expression COLON assignment_expression */
      case 18: /* logical_or_expression ::= logical_and_expression */
      case 20: /* logical_and_expression ::= bitwise_or_expression */
      case 71: /* primary_expression ::= object_literal */
      case 73: /* primary_expression ::= IDENTIFIER */
      case 74: /* primary_expression ::= array_literal */
      case 75: /* primary_expression ::= DECIMAL */
      case 76: /* primary_expression ::= HEX_INTEGER */
      case 77: /* primary_expression ::= STRING */
      case 78: /* primary_expression ::= BOOLEAN */
      case 79: /* primary_expression ::= NULL */
      case 80: /* array_literal ::= BRACKETL elision BRACKETR */
      case 81: /* array_literal ::= BRACKETL element_list elision BRACKETR */
      case 82: /* array_literal ::= BRACKETL element_list BRACKETR */
      case 83: /* elision ::= COMMA */
      case 84: /* elision ::= elision COMMA */
      case 85: /* element_list ::= assignment_expression */
      case 86: /* element_list ::= elision assignment_expression */
      case 87: /* element_list ::= element_list elision assignment_expression */
      case 88: /* object_literal ::= BRACEL property_name_and_value_list RBRACE */
      case 89: /* property_name_and_value_list ::= */
      case 90: /* property_name_and_value_list ::= property_name_and_value_list COMMA property_name_and_value */
      case 91: /* property_name_and_value ::= property_name COLON assignment_expression */
      case 92: /* property_name ::= IDENTIFIER|STRING|DECIMAL */
      case 93: /* member_expression_part ::= BRACKETL expression BRACKETR */
      case 94: /* member_expression_part ::= DOT IDENTIFIER */
      case 95: /* arguments ::= PARENL argument_list PARENR */
      case 96: /* argument_list ::= */
      case 97: /* argument_list ::= argument_list COMMA assignment_expression */
#line 18 "expr.y"
{
}
#line 1116 "expr.c"
        break;
      case 19: /* logical_or_expression ::= logical_or_expression LOGICAL_OR logical_and_expression */
#line 41 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_OR, 2);
}
#line 1123 "expr.c"
        break;
      case 21: /* logical_and_expression ::= logical_and_expression LOGICAL_AND bitwise_or_expression */
#line 46 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND, 2);
}
#line 1130 "expr.c"
        break;
      case 22: /* logical_and_expression ::= logical_and_expression LOGICAL_BUT bitwise_or_expression */
#line 49 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BUT, 2);
}
#line 1137 "expr.c"
        break;
      case 23: /* bitwise_or_expression ::= bitwise_xor_expression */
#line 53 "expr.y"
{
  if (!yymsp[0].minor.yy0) {
    grn_obj *column, *token;
    GRN_PTR_POP(&efsi->token_stack, token);
    column = grn_ptr_value_at(&efsi->column_stack, -1);
    grn_expr_append_obj(efsi->ctx, efsi->e, efsi->v);
    grn_expr_append_const(efsi->ctx, efsi->e, column);
    grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_OBJ_GET_VALUE, 2);
    grn_expr_append_code(efsi->ctx, (grn_expr *)efsi->e, token, GRN_OP_PUSH);
    grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MATCH, 2);
  }
  yygotominor.yy0 = 1;
}
#line 1154 "expr.c"
        break;
      case 24: /* bitwise_or_expression ::= bitwise_or_expression BITWISE_OR bitwise_xor_expression */
      case 26: /* bitwise_xor_expression ::= bitwise_xor_expression BITWISE_XOR bitwise_and_expression */
      case 28: /* bitwise_and_expression ::= bitwise_and_expression BITWISE_AND equality_expression */
      case 30: /* equality_expression ::= equality_expression EQUAL relational_expression */
      case 31: /* equality_expression ::= equality_expression NOT_EQUAL relational_expression */
      case 33: /* relational_expression ::= relational_expression LESS shift_expression */
      case 34: /* relational_expression ::= relational_expression GREATER shift_expression */
      case 35: /* relational_expression ::= relational_expression LESS_EQUAL shift_expression */
      case 36: /* relational_expression ::= relational_expression GREATER_EQUAL shift_expression */
      case 37: /* relational_expression ::= relational_expression IN shift_expression */
      case 38: /* relational_expression ::= relational_expression MATCH shift_expression */
      case 40: /* shift_expression ::= shift_expression SHIFTL additive_expression */
      case 41: /* shift_expression ::= shift_expression SHIFTR additive_expression */
      case 42: /* shift_expression ::= shift_expression SHIFTRR additive_expression */
      case 44: /* additive_expression ::= additive_expression PLUS multiplicative_expression */
      case 45: /* additive_expression ::= additive_expression MINUS multiplicative_expression */
      case 47: /* multiplicative_expression ::= multiplicative_expression STAR unary_expression */
      case 48: /* multiplicative_expression ::= multiplicative_expression SLASH unary_expression */
      case 49: /* multiplicative_expression ::= multiplicative_expression MOD unary_expression */
#line 66 "expr.y"
{ yygotominor.yy0 = yymsp[-2].minor.yy0;}
#line 1177 "expr.c"
        break;
      case 25: /* bitwise_xor_expression ::= bitwise_and_expression */
      case 27: /* bitwise_and_expression ::= equality_expression */
      case 29: /* equality_expression ::= relational_expression */
      case 32: /* relational_expression ::= shift_expression */
      case 39: /* shift_expression ::= additive_expression */
      case 43: /* additive_expression ::= multiplicative_expression */
      case 46: /* multiplicative_expression ::= unary_expression */
      case 50: /* unary_expression ::= postfix_expression */
      case 51: /* unary_expression ::= DELETE unary_expression */
      case 52: /* unary_expression ::= INCR unary_expression */
      case 53: /* unary_expression ::= DECR unary_expression */
      case 54: /* unary_expression ::= PLUS unary_expression */
      case 55: /* unary_expression ::= MINUS unary_expression */
      case 56: /* unary_expression ::= NOT unary_expression */
      case 57: /* unary_expression ::= ADJ_INC unary_expression */
      case 58: /* unary_expression ::= ADJ_DEC unary_expression */
      case 59: /* unary_expression ::= ADJ_NEG unary_expression */
      case 60: /* unary_expression ::= UNARY_SIMILAR unary_expression */
      case 61: /* unary_expression ::= UNARY_EXTRACT unary_expression */
      case 62: /* unary_expression ::= UNARY_NEAR unary_expression */
      case 63: /* postfix_expression ::= lefthand_side_expression */
      case 66: /* lefthand_side_expression ::= call_expression */
      case 67: /* lefthand_side_expression ::= member_expression */
      case 68: /* call_expression ::= member_expression arguments */
      case 69: /* member_expression ::= primary_expression */
#line 68 "expr.y"
{ yygotominor.yy0 = yymsp[0].minor.yy0;}
#line 1206 "expr.c"
        break;
      case 64: /* postfix_expression ::= lefthand_side_expression INCR */
      case 65: /* postfix_expression ::= lefthand_side_expression DECR */
      case 70: /* member_expression ::= member_expression member_expression_part */
#line 115 "expr.y"
{ yygotominor.yy0 = yymsp[-1].minor.yy0;}
#line 1213 "expr.c"
        break;
      case 72: /* primary_expression ::= PARENL expression PARENR */
#line 127 "expr.y"
{ yygotominor.yy0 = 1;}
#line 1218 "expr.c"
        break;
  };
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,yygoto);
  if( yyact < YYNSTATE ){
#ifdef NDEBUG
    /* If we are not debugging and the reduce action popped at least
    ** one element off the stack, then we can push the new element back
    ** onto the stack here, and skip the stack overflow test in yy_shift().
    ** That gives a significant speed improvement. */
    if( yysize ){
      yypParser->yyidx++;
      yymsp -= yysize-1;
      yymsp->stateno = yyact;
      yymsp->major = yygoto;
      yymsp->minor = yygotominor;
    }else
#endif
    {
      yy_shift(yypParser,yyact,yygoto,&yygotominor);
    }
  }else{
    assert( yyact == YYNSTATE + YYNRULE + 1 );
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  grn_expr_parserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
  grn_expr_parserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  YYMINORTYPE yyminor            /* The minor type of the error token */
){
  grn_expr_parserARG_FETCH;
#define TOKEN (yyminor.yy0)
#line 11 "expr.y"

  {
    grn_ctx *ctx = efsi->ctx;
    ERR(GRN_SYNTAX_ERROR, "Syntax error!");
  }
#line 1284 "expr.c"
  grn_expr_parserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  grn_expr_parserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
  grn_expr_parserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "grn_expr_parserAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void grn_expr_parser(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  grn_expr_parserTOKENTYPE yyminor       /* The value for the token */
  grn_expr_parserARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  int yyact;            /* The parser action. */
  int yyendofinput;     /* True if we are at the end of input */
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (yyParser*)yyp;
  if( yypParser->yyidx<0 ){
#if YYSTACKDEPTH<=0
    if( yypParser->yystksz <=0 ){
      /*memset(&yyminorunion, 0, sizeof(yyminorunion));*/
      yyminorunion = yyzerominor;
      yyStackOverflow(yypParser, &yyminorunion);
      return;
    }
#endif
    yypParser->yyidx = 0;
    yypParser->yyerrcnt = -1;
    yypParser->yystack[0].stateno = 0;
    yypParser->yystack[0].major = 0;
  }
  yyminorunion.yy0 = yyminor;
  yyendofinput = (yymajor==0);
  grn_expr_parserARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput %s\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,yymajor);
    if( yyact<YYNSTATE ){
      assert( !yyendofinput );  /* Impossible to shift the $ token */
      yy_shift(yypParser,yyact,yymajor,&yyminorunion);
      yypParser->yyerrcnt--;
      yymajor = YYNOCODE;
    }else if( yyact < YYNSTATE + YYNRULE ){
      yy_reduce(yypParser,yyact-YYNSTATE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        yypParser->yystack[yypParser->yyidx].stateno,
                        YYERRORSYMBOL)) >= YYNSTATE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          yy_destructor(yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(yypParser,yyact,YYERRORSYMBOL,&u2);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
  return;
}
