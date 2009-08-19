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
#define YYNOCODE 99
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
#define YYNSTATE 192
#define YYNRULE 111
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
 /*     0 */   304,   65,  106,  122,  150,  147,  128,   74,   92,   93,
 /*    10 */   112,  113,  114,   98,   77,   84,  102,   83,  161,  132,
 /*    20 */   144,   63,  166,  115,  151,  155,   66,  100,  122,  150,
 /*    30 */   147,  128,   74,   92,   93,  112,  113,  114,   98,   77,
 /*    40 */    84,  102,   83,  161,  132,  144,   63,   27,  119,  151,
 /*    50 */   155,  150,  173,  128,   74,   92,   93,  112,  113,  114,
 /*    60 */    98,   77,   84,  102,   83,  161,  132,  144,   63,   12,
 /*    70 */     7,  151,  155,    8,   72,   91,    9,  150,  147,  128,
 /*    80 */    74,   92,   93,  112,  113,  114,   98,   77,   84,  102,
 /*    90 */    83,  161,  132,  144,   63,   28,   29,  151,  155,  121,
 /*   100 */     1,  103,   69,  118,    3,   91,   61,  150,  147,  128,
 /*   110 */    74,   92,   93,  112,  113,  114,   98,   77,   84,  102,
 /*   120 */    83,  161,  132,  144,   63,  120,    5,  151,  155,  116,
 /*   130 */    35,   36,   37,   38,   39,   40,   41,   42,   43,   53,
 /*   140 */    54,   44,   45,   46,   50,   51,   52,   55,   56,   57,
 /*   150 */    58,  156,  157,  158,  159,  160,    2,   97,  103,  150,
 /*   160 */   147,  128,   74,   92,   93,  112,  113,  114,   98,   77,
 /*   170 */    84,  102,   83,  161,  132,  144,   63,   33,   34,  151,
 /*   180 */   155,  100,   13,  150,  147,  128,   74,   92,   93,  112,
 /*   190 */   113,  114,   98,   77,   84,  102,   83,  161,  132,  144,
 /*   200 */    63,  152,    6,  151,  155,  154,   73,   10,  189,  163,
 /*   210 */   150,  177,  128,   74,   92,   93,  112,  113,  114,   98,
 /*   220 */    77,   84,  102,   83,  161,  132,  144,   63,  151,  155,
 /*   230 */   151,  155,   49,   59,   60,  172,   66,  104,  122,  145,
 /*   240 */   146,   53,   54,  126,  123,   10,   50,   51,   52,   55,
 /*   250 */    56,   57,   58,  156,  157,  158,  159,  160,    2,  162,
 /*   260 */   150,  164,  128,   74,   92,   93,  112,  113,  114,   98,
 /*   270 */    77,   84,  102,   83,  161,  132,  144,   63,  175,    6,
 /*   280 */   151,  155,  154,   73,   15,  171,  163,  150,  170,  128,
 /*   290 */    74,   92,   93,  112,  113,  114,   98,   77,   84,  102,
 /*   300 */    83,  161,  132,  144,   63,   47,   48,  151,  155,  121,
 /*   310 */    62,  124,   69,  107,    4,  125,   61,  190,   53,   54,
 /*   320 */   142,  143,   70,   50,   51,   52,   55,   56,   57,   58,
 /*   330 */   156,  157,  158,  159,  160,    2,  169,  150,  127,  128,
 /*   340 */    74,   92,   93,  112,  113,  114,   98,   77,   84,  102,
 /*   350 */    83,  161,  132,  144,   63,   30,   32,  151,  155,  150,
 /*   360 */   129,  128,   74,   92,   93,  112,  113,  114,   98,   77,
 /*   370 */    84,  102,   83,  161,  132,  144,   63,   31,   64,  151,
 /*   380 */   155,   14,  174,  305,  305,  305,  305,  305,  305,  150,
 /*   390 */   108,  128,   74,   92,   93,  112,  113,  114,   98,   77,
 /*   400 */    84,  102,   83,  161,  132,  144,   63,  168,   10,  151,
 /*   410 */   155,  150,  130,  128,   74,   92,   93,  112,  113,  114,
 /*   420 */    98,   77,   84,  102,   83,  161,  132,  144,   63,  165,
 /*   430 */   305,  151,  155,  150,  167,  128,   74,   92,   93,  112,
 /*   440 */   113,  114,   98,   77,   84,  102,   83,  161,  132,  144,
 /*   450 */    63,  305,  305,  151,  155,  168,  305,  168,  148,  305,
 /*   460 */   150,  176,  128,   74,   92,   93,  112,  113,  114,   98,
 /*   470 */    77,   84,  102,   83,  161,  132,  144,   63,  305,  305,
 /*   480 */   151,  155,  150,  178,  128,   74,   92,   93,  112,  113,
 /*   490 */   114,   98,   77,   84,  102,   83,  161,  132,  144,   63,
 /*   500 */   305,  305,  151,  155,  150,  179,  128,   74,   92,   93,
 /*   510 */   112,  113,  114,   98,   77,   84,  102,   83,  161,  132,
 /*   520 */   144,   63,  305,  305,  151,  155,  305,  305,  305,  305,
 /*   530 */   305,  150,  180,  128,   74,   92,   93,  112,  113,  114,
 /*   540 */    98,   77,   84,  102,   83,  161,  132,  144,   63,  305,
 /*   550 */   305,  151,  155,  150,  181,  128,   74,   92,   93,  112,
 /*   560 */   113,  114,   98,   77,   84,  102,   83,  161,  132,  144,
 /*   570 */    63,  305,  305,  151,  155,  150,  182,  128,   74,   92,
 /*   580 */    93,  112,  113,  114,   98,   77,   84,  102,   83,  161,
 /*   590 */   132,  144,   63,  305,  305,  151,  155,  305,  305,  305,
 /*   600 */   305,  305,  150,  183,  128,   74,   92,   93,  112,  113,
 /*   610 */   114,   98,   77,   84,  102,   83,  161,  132,  144,   63,
 /*   620 */   305,  305,  151,  155,  150,  184,  128,   74,   92,   93,
 /*   630 */   112,  113,  114,   98,   77,   84,  102,   83,  161,  132,
 /*   640 */   144,   63,  305,  305,  151,  155,  150,  185,  128,   74,
 /*   650 */    92,   93,  112,  113,  114,   98,   77,   84,  102,   83,
 /*   660 */   161,  132,  144,   63,  305,  305,  151,  155,  305,  305,
 /*   670 */   305,  305,  305,  150,  186,  128,   74,   92,   93,  112,
 /*   680 */   113,  114,   98,   77,   84,  102,   83,  161,  132,  144,
 /*   690 */    63,  305,  305,  151,  155,  150,  187,  128,   74,   92,
 /*   700 */    93,  112,  113,  114,   98,   77,   84,  102,   83,  161,
 /*   710 */   132,  144,   63,  305,  305,  151,  155,  150,  188,  128,
 /*   720 */    74,   92,   93,  112,  113,  114,   98,   77,   84,  102,
 /*   730 */    83,  161,  132,  144,   63,  305,    6,  151,  155,  154,
 /*   740 */    73,  305,  305,  172,  150,  305,  305,   96,  305,  105,
 /*   750 */   112,  113,  114,   98,   77,   84,  102,   83,  161,  132,
 /*   760 */   144,   63,  193,  305,  151,  155,  305,  305,  305,  305,
 /*   770 */   305,  305,  305,  305,   10,   53,   54,  305,  305,  305,
 /*   780 */    50,   51,   52,   55,   56,   57,   58,  156,  157,  158,
 /*   790 */   159,  160,    2,    6,  305,  150,  154,   73,   96,  305,
 /*   800 */   305,  109,  113,  114,   98,   77,   84,  102,   83,  161,
 /*   810 */   132,  144,   63,  305,  150,  151,  155,   96,  305,  305,
 /*   820 */   117,  113,  114,   98,   77,   84,  102,   83,  161,  132,
 /*   830 */   144,   63,   53,   54,  151,  155,  305,   50,   51,   52,
 /*   840 */    55,   56,   57,   58,  156,  157,  158,  159,  160,    2,
 /*   850 */   305,  150,  305,  305,   96,  305,  305,  305,  110,  114,
 /*   860 */    98,   77,   84,  102,   83,  161,  132,  144,   63,  305,
 /*   870 */   305,  151,  155,  150,  305,  305,   96,  305,  305,  305,
 /*   880 */   305,  111,   98,   77,   84,  102,   83,  161,  132,  144,
 /*   890 */    63,  305,  305,  151,  155,   11,   16,   17,   18,   19,
 /*   900 */    20,   21,   22,   23,   24,   25,   26,  305,  150,  305,
 /*   910 */   305,   96,  305,  305,  305,  305,  305,   94,   77,   84,
 /*   920 */   102,   83,  161,  132,  144,   63,  305,  305,  151,  155,
 /*   930 */   305,  150,  142,  143,   96,  305,  305,  305,  305,  305,
 /*   940 */   305,   75,   84,  102,   83,  161,  132,  144,   63,  305,
 /*   950 */   305,  151,  155,  150,  305,  305,   96,  305,  305,  305,
 /*   960 */   305,  305,  305,   76,   84,  102,   83,  161,  132,  144,
 /*   970 */    63,  305,  150,  151,  155,   96,  305,  305,  305,  305,
 /*   980 */   305,  305,  305,   78,  102,   83,  161,  132,  144,   63,
 /*   990 */   305,  150,  151,  155,   96,  305,  305,  305,  305,  305,
 /*  1000 */   305,  305,   80,  102,   83,  161,  132,  144,   63,  305,
 /*  1010 */   305,  151,  155,  150,  305,  305,   96,  305,  305,  305,
 /*  1020 */   305,  305,  305,  305,   82,  102,   83,  161,  132,  144,
 /*  1030 */    63,  305,  305,  151,  155,  150,  305,  305,   96,  305,
 /*  1040 */   305,  305,  305,  305,  305,  305,   85,  102,   83,  161,
 /*  1050 */   132,  144,   63,  305,  150,  151,  155,   96,  305,  305,
 /*  1060 */   305,  305,  305,  305,  305,   86,  102,   83,  161,  132,
 /*  1070 */   144,   63,  305,  150,  151,  155,   96,  305,  305,  305,
 /*  1080 */   305,  305,  305,  305,   87,  102,   83,  161,  132,  144,
 /*  1090 */    63,  305,  305,  151,  155,  150,  305,  305,   96,  305,
 /*  1100 */   305,  305,  305,  305,  305,  305,   88,  102,   83,  161,
 /*  1110 */   132,  144,   63,  305,  305,  151,  155,  150,  305,  305,
 /*  1120 */    96,  305,  305,  305,  305,  305,  305,  305,   89,  102,
 /*  1130 */    83,  161,  132,  144,   63,  305,  150,  151,  155,   96,
 /*  1140 */   305,  305,  305,  305,  305,  305,  305,   90,  102,   83,
 /*  1150 */   161,  132,  144,   63,  305,  150,  151,  155,   96,  305,
 /*  1160 */   305,  305,  305,  305,  305,  305,  305,   95,   83,  161,
 /*  1170 */   132,  144,   63,  305,  305,  151,  155,  150,  305,  305,
 /*  1180 */    96,  305,  305,  305,  305,  305,  305,  305,  305,   99,
 /*  1190 */    83,  161,  132,  144,   63,  305,  305,  151,  155,  150,
 /*  1200 */   305,  305,   96,  305,  305,  305,  305,  305,  305,  305,
 /*  1210 */   305,  101,   83,  161,  132,  144,   63,  305,  305,  151,
 /*  1220 */   155,  192,   67,   68,   71,  121,   62,  305,   69,  107,
 /*  1230 */     4,  305,   61,  305,   67,   68,   71,  121,   62,  191,
 /*  1240 */    69,  107,    4,  150,   61,    6,   96,  305,  154,   73,
 /*  1250 */   305,  305,  305,  305,  305,  305,   79,  161,  132,  144,
 /*  1260 */    63,  305,  150,  151,  155,   96,  305,  305,  305,  305,
 /*  1270 */   150,  305,  305,   96,  305,   81,  161,  132,  144,   63,
 /*  1280 */   305,  305,  151,  155,  131,  132,  144,   63,  305,  305,
 /*  1290 */   151,  155,  150,  305,  305,   96,  156,  157,  158,  159,
 /*  1300 */   160,    2,  305,  305,  305,  305,  133,  132,  144,   63,
 /*  1310 */   305,  150,  151,  155,   96,  305,  305,  305,  305,  150,
 /*  1320 */   305,  305,   96,  305,  305,  134,  132,  144,   63,  305,
 /*  1330 */   305,  151,  155,  135,  132,  144,   63,  305,  305,  151,
 /*  1340 */   155,  150,  305,  305,   96,  305,  305,  305,  305,  150,
 /*  1350 */   305,  305,   96,  305,  305,  136,  132,  144,   63,  305,
 /*  1360 */   305,  151,  155,  137,  132,  144,   63,  305,  150,  151,
 /*  1370 */   155,   96,  305,  305,  305,  305,  305,  150,  305,  305,
 /*  1380 */    96,  305,  138,  132,  144,   63,  305,  305,  151,  155,
 /*  1390 */   305,  139,  132,  144,   63,  305,  150,  151,  155,   96,
 /*  1400 */   305,  305,  305,  305,  150,  305,  305,   96,  305,  305,
 /*  1410 */   140,  132,  144,   63,  305,  305,  151,  155,  141,  132,
 /*  1420 */   144,   63,  305,  150,  151,  155,   96,  305,  305,  305,
 /*  1430 */   305,  305,  150,  305,  305,   96,  305,  149,  132,  144,
 /*  1440 */    63,  305,  305,  151,  155,  305,  153,  132,  144,   63,
 /*  1450 */   305,  305,  151,  155,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    66,   67,   68,   69,   70,   71,   72,   73,   74,   75,
 /*    10 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   85,
 /*    20 */    86,   87,   95,   96,   90,   91,   67,   68,   69,   70,
 /*    30 */    71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*    40 */    81,   82,   83,   84,   85,   86,   87,    3,   69,   90,
 /*    50 */    91,   70,   71,   72,   73,   74,   75,   76,   77,   78,
 /*    60 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   25,
 /*    70 */     5,   90,   91,   92,   93,   68,   92,   70,   71,   72,
 /*    80 */    73,   74,   75,   76,   77,   78,   79,   80,   81,   82,
 /*    90 */    83,   84,   85,   86,   87,    1,    2,   90,   91,    4,
 /*   100 */     5,   94,    7,    8,    9,   68,   11,   70,   71,   72,
 /*   110 */    73,   74,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   120 */    83,   84,   85,   86,   87,   69,   61,   90,   91,   64,
 /*   130 */    32,   33,   34,   35,   36,   37,   38,   39,   40,   44,
 /*   140 */    45,   41,   42,   43,   49,   50,   51,   52,   53,   54,
 /*   150 */    55,   56,   57,   58,   59,   60,   61,   68,   94,   70,
 /*   160 */    71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*   170 */    81,   82,   83,   84,   85,   86,   87,   30,   31,   90,
 /*   180 */    91,   68,   26,   70,   71,   72,   73,   74,   75,   76,
 /*   190 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   200 */    87,    6,    5,   90,   91,    8,    9,   12,   70,   12,
 /*   210 */    70,   71,   72,   73,   74,   75,   76,   77,   78,   79,
 /*   220 */    80,   81,   82,   83,   84,   85,   86,   87,   90,   91,
 /*   230 */    90,   91,   46,   47,   48,   12,   67,   97,   69,   88,
 /*   240 */    89,   44,   45,   10,   69,   12,   49,   50,   51,   52,
 /*   250 */    53,   54,   55,   56,   57,   58,   59,   60,   61,   62,
 /*   260 */    70,   71,   72,   73,   74,   75,   76,   77,   78,   79,
 /*   270 */    80,   81,   82,   83,   84,   85,   86,   87,    6,    5,
 /*   280 */    90,   91,    8,    9,   12,   62,   12,   70,   71,   72,
 /*   290 */    73,   74,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   300 */    83,   84,   85,   86,   87,   44,   45,   90,   91,    4,
 /*   310 */     5,   69,    7,    8,    9,   69,   11,   69,   44,   45,
 /*   320 */    50,   51,    7,   49,   50,   51,   52,   53,   54,   55,
 /*   330 */    56,   57,   58,   59,   60,   61,   62,   70,   71,   72,
 /*   340 */    73,   74,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   350 */    83,   84,   85,   86,   87,   27,   29,   90,   91,   70,
 /*   360 */    71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*   370 */    81,   82,   83,   84,   85,   86,   87,   28,   12,   90,
 /*   380 */    91,   26,    8,   98,   98,   98,   98,   98,   98,   70,
 /*   390 */    71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*   400 */    81,   82,   83,   84,   85,   86,   87,    8,   12,   90,
 /*   410 */    91,   70,   71,   72,   73,   74,   75,   76,   77,   78,
 /*   420 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   63,
 /*   430 */    98,   90,   91,   70,   71,   72,   73,   74,   75,   76,
 /*   440 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   450 */    87,   98,   98,   90,   91,   56,   98,   58,   62,   98,
 /*   460 */    70,   71,   72,   73,   74,   75,   76,   77,   78,   79,
 /*   470 */    80,   81,   82,   83,   84,   85,   86,   87,   98,   98,
 /*   480 */    90,   91,   70,   71,   72,   73,   74,   75,   76,   77,
 /*   490 */    78,   79,   80,   81,   82,   83,   84,   85,   86,   87,
 /*   500 */    98,   98,   90,   91,   70,   71,   72,   73,   74,   75,
 /*   510 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   85,
 /*   520 */    86,   87,   98,   98,   90,   91,   98,   98,   98,   98,
 /*   530 */    98,   70,   71,   72,   73,   74,   75,   76,   77,   78,
 /*   540 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   98,
 /*   550 */    98,   90,   91,   70,   71,   72,   73,   74,   75,   76,
 /*   560 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   570 */    87,   98,   98,   90,   91,   70,   71,   72,   73,   74,
 /*   580 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   590 */    85,   86,   87,   98,   98,   90,   91,   98,   98,   98,
 /*   600 */    98,   98,   70,   71,   72,   73,   74,   75,   76,   77,
 /*   610 */    78,   79,   80,   81,   82,   83,   84,   85,   86,   87,
 /*   620 */    98,   98,   90,   91,   70,   71,   72,   73,   74,   75,
 /*   630 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   85,
 /*   640 */    86,   87,   98,   98,   90,   91,   70,   71,   72,   73,
 /*   650 */    74,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   660 */    84,   85,   86,   87,   98,   98,   90,   91,   98,   98,
 /*   670 */    98,   98,   98,   70,   71,   72,   73,   74,   75,   76,
 /*   680 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   690 */    87,   98,   98,   90,   91,   70,   71,   72,   73,   74,
 /*   700 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   710 */    85,   86,   87,   98,   98,   90,   91,   70,   71,   72,
 /*   720 */    73,   74,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   730 */    83,   84,   85,   86,   87,   98,    5,   90,   91,    8,
 /*   740 */     9,   98,   98,   12,   70,   98,   98,   73,   98,   75,
 /*   750 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   85,
 /*   760 */    86,   87,    0,   98,   90,   91,   98,   98,   98,   98,
 /*   770 */    98,   98,   98,   98,   12,   44,   45,   98,   98,   98,
 /*   780 */    49,   50,   51,   52,   53,   54,   55,   56,   57,   58,
 /*   790 */    59,   60,   61,    5,   98,   70,    8,    9,   73,   98,
 /*   800 */    98,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   810 */    85,   86,   87,   98,   70,   90,   91,   73,   98,   98,
 /*   820 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   85,
 /*   830 */    86,   87,   44,   45,   90,   91,   98,   49,   50,   51,
 /*   840 */    52,   53,   54,   55,   56,   57,   58,   59,   60,   61,
 /*   850 */    98,   70,   98,   98,   73,   98,   98,   98,   77,   78,
 /*   860 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   98,
 /*   870 */    98,   90,   91,   70,   98,   98,   73,   98,   98,   98,
 /*   880 */    98,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   890 */    87,   98,   98,   90,   91,   13,   14,   15,   16,   17,
 /*   900 */    18,   19,   20,   21,   22,   23,   24,   98,   70,   98,
 /*   910 */    98,   73,   98,   98,   98,   98,   98,   79,   80,   81,
 /*   920 */    82,   83,   84,   85,   86,   87,   98,   98,   90,   91,
 /*   930 */    98,   70,   50,   51,   73,   98,   98,   98,   98,   98,
 /*   940 */    98,   80,   81,   82,   83,   84,   85,   86,   87,   98,
 /*   950 */    98,   90,   91,   70,   98,   98,   73,   98,   98,   98,
 /*   960 */    98,   98,   98,   80,   81,   82,   83,   84,   85,   86,
 /*   970 */    87,   98,   70,   90,   91,   73,   98,   98,   98,   98,
 /*   980 */    98,   98,   98,   81,   82,   83,   84,   85,   86,   87,
 /*   990 */    98,   70,   90,   91,   73,   98,   98,   98,   98,   98,
 /*  1000 */    98,   98,   81,   82,   83,   84,   85,   86,   87,   98,
 /*  1010 */    98,   90,   91,   70,   98,   98,   73,   98,   98,   98,
 /*  1020 */    98,   98,   98,   98,   81,   82,   83,   84,   85,   86,
 /*  1030 */    87,   98,   98,   90,   91,   70,   98,   98,   73,   98,
 /*  1040 */    98,   98,   98,   98,   98,   98,   81,   82,   83,   84,
 /*  1050 */    85,   86,   87,   98,   70,   90,   91,   73,   98,   98,
 /*  1060 */    98,   98,   98,   98,   98,   81,   82,   83,   84,   85,
 /*  1070 */    86,   87,   98,   70,   90,   91,   73,   98,   98,   98,
 /*  1080 */    98,   98,   98,   98,   81,   82,   83,   84,   85,   86,
 /*  1090 */    87,   98,   98,   90,   91,   70,   98,   98,   73,   98,
 /*  1100 */    98,   98,   98,   98,   98,   98,   81,   82,   83,   84,
 /*  1110 */    85,   86,   87,   98,   98,   90,   91,   70,   98,   98,
 /*  1120 */    73,   98,   98,   98,   98,   98,   98,   98,   81,   82,
 /*  1130 */    83,   84,   85,   86,   87,   98,   70,   90,   91,   73,
 /*  1140 */    98,   98,   98,   98,   98,   98,   98,   81,   82,   83,
 /*  1150 */    84,   85,   86,   87,   98,   70,   90,   91,   73,   98,
 /*  1160 */    98,   98,   98,   98,   98,   98,   98,   82,   83,   84,
 /*  1170 */    85,   86,   87,   98,   98,   90,   91,   70,   98,   98,
 /*  1180 */    73,   98,   98,   98,   98,   98,   98,   98,   98,   82,
 /*  1190 */    83,   84,   85,   86,   87,   98,   98,   90,   91,   70,
 /*  1200 */    98,   98,   73,   98,   98,   98,   98,   98,   98,   98,
 /*  1210 */    98,   82,   83,   84,   85,   86,   87,   98,   98,   90,
 /*  1220 */    91,    0,    1,    2,    3,    4,    5,   98,    7,    8,
 /*  1230 */     9,   98,   11,   98,    1,    2,    3,    4,    5,    6,
 /*  1240 */     7,    8,    9,   70,   11,    5,   73,   98,    8,    9,
 /*  1250 */    98,   98,   98,   98,   98,   98,   83,   84,   85,   86,
 /*  1260 */    87,   98,   70,   90,   91,   73,   98,   98,   98,   98,
 /*  1270 */    70,   98,   98,   73,   98,   83,   84,   85,   86,   87,
 /*  1280 */    98,   98,   90,   91,   84,   85,   86,   87,   98,   98,
 /*  1290 */    90,   91,   70,   98,   98,   73,   56,   57,   58,   59,
 /*  1300 */    60,   61,   98,   98,   98,   98,   84,   85,   86,   87,
 /*  1310 */    98,   70,   90,   91,   73,   98,   98,   98,   98,   70,
 /*  1320 */    98,   98,   73,   98,   98,   84,   85,   86,   87,   98,
 /*  1330 */    98,   90,   91,   84,   85,   86,   87,   98,   98,   90,
 /*  1340 */    91,   70,   98,   98,   73,   98,   98,   98,   98,   70,
 /*  1350 */    98,   98,   73,   98,   98,   84,   85,   86,   87,   98,
 /*  1360 */    98,   90,   91,   84,   85,   86,   87,   98,   70,   90,
 /*  1370 */    91,   73,   98,   98,   98,   98,   98,   70,   98,   98,
 /*  1380 */    73,   98,   84,   85,   86,   87,   98,   98,   90,   91,
 /*  1390 */    98,   84,   85,   86,   87,   98,   70,   90,   91,   73,
 /*  1400 */    98,   98,   98,   98,   70,   98,   98,   73,   98,   98,
 /*  1410 */    84,   85,   86,   87,   98,   98,   90,   91,   84,   85,
 /*  1420 */    86,   87,   98,   70,   90,   91,   73,   98,   98,   98,
 /*  1430 */    98,   98,   70,   98,   98,   73,   98,   84,   85,   86,
 /*  1440 */    87,   98,   98,   90,   91,   98,   84,   85,   86,   87,
 /*  1450 */    98,   98,   90,   91,
};
#define YY_SHIFT_USE_DFLT (-1)
#define YY_SHIFT_MAX 118
static const short yy_shift_ofst[] = {
 /*     0 */    95,   95,  731,  788,  788,  788,  788,  788,  197,  274,
 /*    10 */   788,  788,  788,  788,  788,  788,  788,  788,  788,  788,
 /*    20 */   788,  788,  788,  788,  788,  788,  788,  788,  788,  788,
 /*    30 */   788,  788,  788,  788,  788,  788,  788,  788,  788,  788,
 /*    40 */   788,  788,  788,  788,  788,  788,  788,  788,  788,  788,
 /*    50 */   788,  788,  788,  788,  788,  788,  788,  788,  788,  788,
 /*    60 */   788, 1240,  305,   65,  399, 1221, 1233,  305,  305,  305,
 /*    70 */   305,  305,  223,   -1,  882,   98,   98,   98,  100,  186,
 /*    80 */   100,  186,  100,  186,  100,  100,  100,  100,  100,  100,
 /*    90 */   100,  233,   44,   94,  147,  261,  270,  396,  147,  261,
 /*   100 */   195,  261,  261,  366,  272,   94,  762,  315,  156,  328,
 /*   110 */   349,  327,  328,  349,  327,  355,  374,  328,  315,
};
#define YY_REDUCE_USE_DFLT (-74)
#define YY_REDUCE_MAX 73
static const short yy_reduce_ofst[] = {
 /*     0 */   -66,  -41,  -19,    7,   37,   89,  113,  140,  190,  217,
 /*    10 */   267,  289,  319,  341,  363,  390,  412,  434,  461,  483,
 /*    20 */   505,  532,  554,  576,  603,  625,  647,  674,  725,  744,
 /*    30 */   781,  803,  838,  861,  883,  902,  921,  943,  965,  984,
 /*    40 */  1003, 1025, 1047, 1066, 1085, 1107, 1129, 1173, 1192, 1200,
 /*    50 */  1222, 1241, 1249, 1271, 1279, 1298, 1307, 1326, 1334, 1353,
 /*    60 */  1362,  138,  169,  151,  -73,  -21,  -21,   56,  175,  242,
 /*    70 */   246,  248,  -16,   64,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   303,  303,  303,  293,  303,  303,  303,  300,  303,  303,
 /*    10 */   303,  303,  303,  303,  303,  303,  303,  303,  303,  303,
 /*    20 */   303,  303,  303,  303,  303,  303,  303,  303,  303,  303,
 /*    30 */   303,  303,  303,  303,  303,  303,  303,  303,  303,  303,
 /*    40 */   303,  303,  303,  303,  303,  303,  303,  303,  303,  303,
 /*    50 */   303,  303,  303,  303,  303,  303,  303,  303,  303,  303,
 /*    60 */   303,  303,  303,  271,  303,  303,  303,  303,  303,  303,
 /*    70 */   303,  303,  303,  293,  267,  234,  235,  233,  237,  251,
 /*    80 */   238,  252,  239,  250,  236,  240,  241,  242,  243,  244,
 /*    90 */   245,  303,  220,  222,  232,  247,  267,  303,  231,  248,
 /*   100 */   303,  249,  246,  303,  303,  223,  303,  303,  303,  225,
 /*   110 */   228,  230,  224,  227,  229,  303,  303,  226,  277,  195,
 /*   120 */   196,  199,  194,  197,  201,  202,  203,  206,  207,  208,
 /*   130 */   221,  254,  257,  258,  259,  260,  261,  262,  263,  264,
 /*   140 */   265,  266,  268,  269,  270,  272,  274,  205,  297,  255,
 /*   150 */   273,  275,  276,  256,  277,  278,  279,  280,  281,  282,
 /*   160 */   283,  253,  284,  288,  290,  292,  294,  295,  296,  285,
 /*   170 */   291,  286,  287,  289,  298,  299,  302,  301,  209,  210,
 /*   180 */   211,  212,  213,  214,  215,  216,  217,  218,  219,  204,
 /*   190 */   198,  200,
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
  "$",             "LOGICAL_AND",   "LOGICAL_BUT",   "LOGICAL_OR",  
  "QSTRING",       "PARENL",        "PARENR",        "RELATIVE_OP", 
  "IDENTIFIER",    "BRACEL",        "BRACER",        "EVAL",        
  "COMMA",         "ASSIGN",        "STAR_ASSIGN",   "SLASH_ASSIGN",
  "MOD_ASSIGN",    "PLUS_ASSIGN",   "MINUS_ASSIGN",  "SHIFTL_ASSIGN",
  "SHIRTR_ASSIGN",  "SHIFTRR_ASSIGN",  "AND_ASSIGN",    "XOR_ASSIGN",  
  "OR_ASSIGN",     "QUESTION",      "COLON",         "BITWISE_OR",  
  "BITWISE_XOR",   "BITWISE_AND",   "EQUAL",         "NOT_EQUAL",   
  "LESS",          "GREATER",       "LESS_EQUAL",    "GREATER_EQUAL",
  "IN",            "MATCH",         "NEAR",          "SIMILAR",     
  "EXTRACT",       "SHIFTL",        "SHIFTR",        "SHIFTRR",     
  "PLUS",          "MINUS",         "STAR",          "SLASH",       
  "MOD",           "DELETE",        "INCR",          "DECR",        
  "NOT",           "ADJ_INC",       "ADJ_DEC",       "ADJ_NEG",     
  "DECIMAL",       "HEX_INTEGER",   "STRING",        "BOOLEAN",     
  "NULL",          "BRACKETL",      "BRACKETR",      "RBRACE",      
  "DOT",           "error",         "input",         "query",       
  "expression",    "query_element",  "primary_expression",  "assignment_expression",
  "conditional_expression",  "lefthand_side_expression",  "logical_or_expression",  "logical_and_expression",
  "bitwise_or_expression",  "bitwise_xor_expression",  "bitwise_and_expression",  "equality_expression",
  "relational_expression",  "shift_expression",  "additive_expression",  "multiplicative_expression",
  "unary_expression",  "postfix_expression",  "call_expression",  "member_expression",
  "arguments",     "member_expression_part",  "object_literal",  "array_literal",
  "elision",       "element_list",  "property_name_and_value_list",  "property_name_and_value",
  "property_name",  "argument_list",
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "input ::= query",
 /*   1 */ "input ::= expression",
 /*   2 */ "query ::= query_element",
 /*   3 */ "query ::= query query_element",
 /*   4 */ "query ::= query LOGICAL_AND query_element",
 /*   5 */ "query ::= query LOGICAL_BUT query_element",
 /*   6 */ "query ::= query LOGICAL_OR query_element",
 /*   7 */ "query_element ::= QSTRING",
 /*   8 */ "query_element ::= PARENL query PARENR",
 /*   9 */ "query_element ::= RELATIVE_OP query_element",
 /*  10 */ "query_element ::= IDENTIFIER RELATIVE_OP query_element",
 /*  11 */ "query_element ::= BRACEL expression BRACER",
 /*  12 */ "query_element ::= EVAL primary_expression",
 /*  13 */ "expression ::= assignment_expression",
 /*  14 */ "expression ::= expression COMMA assignment_expression",
 /*  15 */ "assignment_expression ::= conditional_expression",
 /*  16 */ "assignment_expression ::= lefthand_side_expression ASSIGN assignment_expression",
 /*  17 */ "assignment_expression ::= lefthand_side_expression STAR_ASSIGN assignment_expression",
 /*  18 */ "assignment_expression ::= lefthand_side_expression SLASH_ASSIGN assignment_expression",
 /*  19 */ "assignment_expression ::= lefthand_side_expression MOD_ASSIGN assignment_expression",
 /*  20 */ "assignment_expression ::= lefthand_side_expression PLUS_ASSIGN assignment_expression",
 /*  21 */ "assignment_expression ::= lefthand_side_expression MINUS_ASSIGN assignment_expression",
 /*  22 */ "assignment_expression ::= lefthand_side_expression SHIFTL_ASSIGN assignment_expression",
 /*  23 */ "assignment_expression ::= lefthand_side_expression SHIRTR_ASSIGN assignment_expression",
 /*  24 */ "assignment_expression ::= lefthand_side_expression SHIFTRR_ASSIGN assignment_expression",
 /*  25 */ "assignment_expression ::= lefthand_side_expression AND_ASSIGN assignment_expression",
 /*  26 */ "assignment_expression ::= lefthand_side_expression XOR_ASSIGN assignment_expression",
 /*  27 */ "assignment_expression ::= lefthand_side_expression OR_ASSIGN assignment_expression",
 /*  28 */ "conditional_expression ::= logical_or_expression",
 /*  29 */ "conditional_expression ::= logical_or_expression QUESTION assignment_expression COLON assignment_expression",
 /*  30 */ "logical_or_expression ::= logical_and_expression",
 /*  31 */ "logical_or_expression ::= logical_or_expression LOGICAL_OR logical_and_expression",
 /*  32 */ "logical_and_expression ::= bitwise_or_expression",
 /*  33 */ "logical_and_expression ::= logical_and_expression LOGICAL_AND bitwise_or_expression",
 /*  34 */ "logical_and_expression ::= logical_and_expression LOGICAL_BUT bitwise_or_expression",
 /*  35 */ "bitwise_or_expression ::= bitwise_xor_expression",
 /*  36 */ "bitwise_or_expression ::= bitwise_or_expression BITWISE_OR bitwise_xor_expression",
 /*  37 */ "bitwise_xor_expression ::= bitwise_and_expression",
 /*  38 */ "bitwise_xor_expression ::= bitwise_xor_expression BITWISE_XOR bitwise_and_expression",
 /*  39 */ "bitwise_and_expression ::= equality_expression",
 /*  40 */ "bitwise_and_expression ::= bitwise_and_expression BITWISE_AND equality_expression",
 /*  41 */ "equality_expression ::= relational_expression",
 /*  42 */ "equality_expression ::= equality_expression EQUAL relational_expression",
 /*  43 */ "equality_expression ::= equality_expression NOT_EQUAL relational_expression",
 /*  44 */ "relational_expression ::= shift_expression",
 /*  45 */ "relational_expression ::= relational_expression LESS shift_expression",
 /*  46 */ "relational_expression ::= relational_expression GREATER shift_expression",
 /*  47 */ "relational_expression ::= relational_expression LESS_EQUAL shift_expression",
 /*  48 */ "relational_expression ::= relational_expression GREATER_EQUAL shift_expression",
 /*  49 */ "relational_expression ::= relational_expression IN shift_expression",
 /*  50 */ "relational_expression ::= relational_expression MATCH shift_expression",
 /*  51 */ "relational_expression ::= relational_expression NEAR shift_expression",
 /*  52 */ "relational_expression ::= relational_expression SIMILAR shift_expression",
 /*  53 */ "relational_expression ::= relational_expression EXTRACT shift_expression",
 /*  54 */ "shift_expression ::= additive_expression",
 /*  55 */ "shift_expression ::= shift_expression SHIFTL additive_expression",
 /*  56 */ "shift_expression ::= shift_expression SHIFTR additive_expression",
 /*  57 */ "shift_expression ::= shift_expression SHIFTRR additive_expression",
 /*  58 */ "additive_expression ::= multiplicative_expression",
 /*  59 */ "additive_expression ::= additive_expression PLUS multiplicative_expression",
 /*  60 */ "additive_expression ::= additive_expression MINUS multiplicative_expression",
 /*  61 */ "multiplicative_expression ::= unary_expression",
 /*  62 */ "multiplicative_expression ::= multiplicative_expression STAR unary_expression",
 /*  63 */ "multiplicative_expression ::= multiplicative_expression SLASH unary_expression",
 /*  64 */ "multiplicative_expression ::= multiplicative_expression MOD unary_expression",
 /*  65 */ "unary_expression ::= postfix_expression",
 /*  66 */ "unary_expression ::= DELETE unary_expression",
 /*  67 */ "unary_expression ::= INCR unary_expression",
 /*  68 */ "unary_expression ::= DECR unary_expression",
 /*  69 */ "unary_expression ::= PLUS unary_expression",
 /*  70 */ "unary_expression ::= MINUS unary_expression",
 /*  71 */ "unary_expression ::= NOT unary_expression",
 /*  72 */ "unary_expression ::= ADJ_INC unary_expression",
 /*  73 */ "unary_expression ::= ADJ_DEC unary_expression",
 /*  74 */ "unary_expression ::= ADJ_NEG unary_expression",
 /*  75 */ "postfix_expression ::= lefthand_side_expression",
 /*  76 */ "postfix_expression ::= lefthand_side_expression INCR",
 /*  77 */ "postfix_expression ::= lefthand_side_expression DECR",
 /*  78 */ "lefthand_side_expression ::= call_expression",
 /*  79 */ "lefthand_side_expression ::= member_expression",
 /*  80 */ "call_expression ::= member_expression arguments",
 /*  81 */ "member_expression ::= primary_expression",
 /*  82 */ "member_expression ::= member_expression member_expression_part",
 /*  83 */ "primary_expression ::= object_literal",
 /*  84 */ "primary_expression ::= PARENL expression PARENR",
 /*  85 */ "primary_expression ::= IDENTIFIER",
 /*  86 */ "primary_expression ::= array_literal",
 /*  87 */ "primary_expression ::= DECIMAL",
 /*  88 */ "primary_expression ::= HEX_INTEGER",
 /*  89 */ "primary_expression ::= STRING",
 /*  90 */ "primary_expression ::= BOOLEAN",
 /*  91 */ "primary_expression ::= NULL",
 /*  92 */ "array_literal ::= BRACKETL elision BRACKETR",
 /*  93 */ "array_literal ::= BRACKETL element_list elision BRACKETR",
 /*  94 */ "array_literal ::= BRACKETL element_list BRACKETR",
 /*  95 */ "elision ::= COMMA",
 /*  96 */ "elision ::= elision COMMA",
 /*  97 */ "element_list ::= assignment_expression",
 /*  98 */ "element_list ::= elision assignment_expression",
 /*  99 */ "element_list ::= element_list elision assignment_expression",
 /* 100 */ "object_literal ::= BRACEL property_name_and_value_list RBRACE",
 /* 101 */ "property_name_and_value_list ::=",
 /* 102 */ "property_name_and_value_list ::= property_name_and_value_list COMMA property_name_and_value",
 /* 103 */ "property_name_and_value ::= property_name COLON assignment_expression",
 /* 104 */ "property_name ::= IDENTIFIER|STRING|DECIMAL",
 /* 105 */ "member_expression_part ::= BRACKETL expression BRACKETR",
 /* 106 */ "member_expression_part ::= DOT IDENTIFIER",
 /* 107 */ "arguments ::= PARENL argument_list PARENR",
 /* 108 */ "argument_list ::=",
 /* 109 */ "argument_list ::= assignment_expression",
 /* 110 */ "argument_list ::= argument_list COMMA assignment_expression",
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
  { 66, 1 },
  { 66, 1 },
  { 67, 1 },
  { 67, 2 },
  { 67, 3 },
  { 67, 3 },
  { 67, 3 },
  { 69, 1 },
  { 69, 3 },
  { 69, 2 },
  { 69, 3 },
  { 69, 3 },
  { 69, 2 },
  { 68, 1 },
  { 68, 3 },
  { 71, 1 },
  { 71, 3 },
  { 71, 3 },
  { 71, 3 },
  { 71, 3 },
  { 71, 3 },
  { 71, 3 },
  { 71, 3 },
  { 71, 3 },
  { 71, 3 },
  { 71, 3 },
  { 71, 3 },
  { 71, 3 },
  { 72, 1 },
  { 72, 5 },
  { 74, 1 },
  { 74, 3 },
  { 75, 1 },
  { 75, 3 },
  { 75, 3 },
  { 76, 1 },
  { 76, 3 },
  { 77, 1 },
  { 77, 3 },
  { 78, 1 },
  { 78, 3 },
  { 79, 1 },
  { 79, 3 },
  { 79, 3 },
  { 80, 1 },
  { 80, 3 },
  { 80, 3 },
  { 80, 3 },
  { 80, 3 },
  { 80, 3 },
  { 80, 3 },
  { 80, 3 },
  { 80, 3 },
  { 80, 3 },
  { 81, 1 },
  { 81, 3 },
  { 81, 3 },
  { 81, 3 },
  { 82, 1 },
  { 82, 3 },
  { 82, 3 },
  { 83, 1 },
  { 83, 3 },
  { 83, 3 },
  { 83, 3 },
  { 84, 1 },
  { 84, 2 },
  { 84, 2 },
  { 84, 2 },
  { 84, 2 },
  { 84, 2 },
  { 84, 2 },
  { 84, 2 },
  { 84, 2 },
  { 84, 2 },
  { 85, 1 },
  { 85, 2 },
  { 85, 2 },
  { 73, 1 },
  { 73, 1 },
  { 86, 2 },
  { 87, 1 },
  { 87, 2 },
  { 70, 1 },
  { 70, 3 },
  { 70, 1 },
  { 70, 1 },
  { 70, 1 },
  { 70, 1 },
  { 70, 1 },
  { 70, 1 },
  { 70, 1 },
  { 91, 3 },
  { 91, 4 },
  { 91, 3 },
  { 92, 1 },
  { 92, 2 },
  { 93, 1 },
  { 93, 2 },
  { 93, 3 },
  { 90, 3 },
  { 94, 0 },
  { 94, 3 },
  { 95, 3 },
  { 96, 1 },
  { 89, 3 },
  { 89, 2 },
  { 88, 3 },
  { 97, 0 },
  { 97, 1 },
  { 97, 3 },
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
      case 0: /* input ::= query */
      case 1: /* input ::= expression */
      case 2: /* query ::= query_element */
      case 7: /* query_element ::= QSTRING */
      case 8: /* query_element ::= PARENL query PARENR */
      case 13: /* expression ::= assignment_expression */
      case 14: /* expression ::= expression COMMA assignment_expression */
      case 15: /* assignment_expression ::= conditional_expression */
      case 16: /* assignment_expression ::= lefthand_side_expression ASSIGN assignment_expression */
      case 17: /* assignment_expression ::= lefthand_side_expression STAR_ASSIGN assignment_expression */
      case 18: /* assignment_expression ::= lefthand_side_expression SLASH_ASSIGN assignment_expression */
      case 19: /* assignment_expression ::= lefthand_side_expression MOD_ASSIGN assignment_expression */
      case 20: /* assignment_expression ::= lefthand_side_expression PLUS_ASSIGN assignment_expression */
      case 21: /* assignment_expression ::= lefthand_side_expression MINUS_ASSIGN assignment_expression */
      case 22: /* assignment_expression ::= lefthand_side_expression SHIFTL_ASSIGN assignment_expression */
      case 23: /* assignment_expression ::= lefthand_side_expression SHIRTR_ASSIGN assignment_expression */
      case 24: /* assignment_expression ::= lefthand_side_expression SHIFTRR_ASSIGN assignment_expression */
      case 25: /* assignment_expression ::= lefthand_side_expression AND_ASSIGN assignment_expression */
      case 26: /* assignment_expression ::= lefthand_side_expression XOR_ASSIGN assignment_expression */
      case 27: /* assignment_expression ::= lefthand_side_expression OR_ASSIGN assignment_expression */
      case 28: /* conditional_expression ::= logical_or_expression */
      case 29: /* conditional_expression ::= logical_or_expression QUESTION assignment_expression COLON assignment_expression */
      case 30: /* logical_or_expression ::= logical_and_expression */
      case 32: /* logical_and_expression ::= bitwise_or_expression */
      case 35: /* bitwise_or_expression ::= bitwise_xor_expression */
      case 36: /* bitwise_or_expression ::= bitwise_or_expression BITWISE_OR bitwise_xor_expression */
      case 37: /* bitwise_xor_expression ::= bitwise_and_expression */
      case 38: /* bitwise_xor_expression ::= bitwise_xor_expression BITWISE_XOR bitwise_and_expression */
      case 39: /* bitwise_and_expression ::= equality_expression */
      case 40: /* bitwise_and_expression ::= bitwise_and_expression BITWISE_AND equality_expression */
      case 41: /* equality_expression ::= relational_expression */
      case 42: /* equality_expression ::= equality_expression EQUAL relational_expression */
      case 43: /* equality_expression ::= equality_expression NOT_EQUAL relational_expression */
      case 44: /* relational_expression ::= shift_expression */
      case 45: /* relational_expression ::= relational_expression LESS shift_expression */
      case 46: /* relational_expression ::= relational_expression GREATER shift_expression */
      case 47: /* relational_expression ::= relational_expression LESS_EQUAL shift_expression */
      case 48: /* relational_expression ::= relational_expression GREATER_EQUAL shift_expression */
      case 49: /* relational_expression ::= relational_expression IN shift_expression */
      case 50: /* relational_expression ::= relational_expression MATCH shift_expression */
      case 51: /* relational_expression ::= relational_expression NEAR shift_expression */
      case 52: /* relational_expression ::= relational_expression SIMILAR shift_expression */
      case 53: /* relational_expression ::= relational_expression EXTRACT shift_expression */
      case 54: /* shift_expression ::= additive_expression */
      case 55: /* shift_expression ::= shift_expression SHIFTL additive_expression */
      case 56: /* shift_expression ::= shift_expression SHIFTR additive_expression */
      case 57: /* shift_expression ::= shift_expression SHIFTRR additive_expression */
      case 58: /* additive_expression ::= multiplicative_expression */
      case 59: /* additive_expression ::= additive_expression PLUS multiplicative_expression */
      case 60: /* additive_expression ::= additive_expression MINUS multiplicative_expression */
      case 61: /* multiplicative_expression ::= unary_expression */
      case 62: /* multiplicative_expression ::= multiplicative_expression STAR unary_expression */
      case 63: /* multiplicative_expression ::= multiplicative_expression SLASH unary_expression */
      case 64: /* multiplicative_expression ::= multiplicative_expression MOD unary_expression */
      case 65: /* unary_expression ::= postfix_expression */
      case 66: /* unary_expression ::= DELETE unary_expression */
      case 67: /* unary_expression ::= INCR unary_expression */
      case 68: /* unary_expression ::= DECR unary_expression */
      case 69: /* unary_expression ::= PLUS unary_expression */
      case 70: /* unary_expression ::= MINUS unary_expression */
      case 71: /* unary_expression ::= NOT unary_expression */
      case 72: /* unary_expression ::= ADJ_INC unary_expression */
      case 73: /* unary_expression ::= ADJ_DEC unary_expression */
      case 74: /* unary_expression ::= ADJ_NEG unary_expression */
      case 75: /* postfix_expression ::= lefthand_side_expression */
      case 76: /* postfix_expression ::= lefthand_side_expression INCR */
      case 77: /* postfix_expression ::= lefthand_side_expression DECR */
      case 78: /* lefthand_side_expression ::= call_expression */
      case 79: /* lefthand_side_expression ::= member_expression */
      case 80: /* call_expression ::= member_expression arguments */
      case 81: /* member_expression ::= primary_expression */
      case 82: /* member_expression ::= member_expression member_expression_part */
      case 83: /* primary_expression ::= object_literal */
      case 84: /* primary_expression ::= PARENL expression PARENR */
      case 85: /* primary_expression ::= IDENTIFIER */
      case 86: /* primary_expression ::= array_literal */
      case 87: /* primary_expression ::= DECIMAL */
      case 88: /* primary_expression ::= HEX_INTEGER */
      case 89: /* primary_expression ::= STRING */
      case 90: /* primary_expression ::= BOOLEAN */
      case 91: /* primary_expression ::= NULL */
      case 92: /* array_literal ::= BRACKETL elision BRACKETR */
      case 93: /* array_literal ::= BRACKETL element_list elision BRACKETR */
      case 94: /* array_literal ::= BRACKETL element_list BRACKETR */
      case 95: /* elision ::= COMMA */
      case 96: /* elision ::= elision COMMA */
      case 97: /* element_list ::= assignment_expression */
      case 98: /* element_list ::= elision assignment_expression */
      case 99: /* element_list ::= element_list elision assignment_expression */
      case 100: /* object_literal ::= BRACEL property_name_and_value_list RBRACE */
      case 101: /* property_name_and_value_list ::= */
      case 102: /* property_name_and_value_list ::= property_name_and_value_list COMMA property_name_and_value */
      case 103: /* property_name_and_value ::= property_name COLON assignment_expression */
      case 104: /* property_name ::= IDENTIFIER|STRING|DECIMAL */
      case 105: /* member_expression_part ::= BRACKETL expression BRACKETR */
      case 106: /* member_expression_part ::= DOT IDENTIFIER */
#line 18 "expr.y"
{
}
#line 1255 "expr.c"
        break;
      case 3: /* query ::= query query_element */
#line 22 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, grn_int32_value_at(&efsi->op_stack, -1), 2);
}
#line 1262 "expr.c"
        break;
      case 4: /* query ::= query LOGICAL_AND query_element */
      case 33: /* logical_and_expression ::= logical_and_expression LOGICAL_AND bitwise_or_expression */
#line 25 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND, 2);
}
#line 1270 "expr.c"
        break;
      case 5: /* query ::= query LOGICAL_BUT query_element */
      case 34: /* logical_and_expression ::= logical_and_expression LOGICAL_BUT bitwise_or_expression */
#line 28 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BUT, 2);
}
#line 1278 "expr.c"
        break;
      case 6: /* query ::= query LOGICAL_OR query_element */
      case 31: /* logical_or_expression ::= logical_or_expression LOGICAL_OR logical_and_expression */
#line 31 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_OR, 2);
}
#line 1286 "expr.c"
        break;
      case 9: /* query_element ::= RELATIVE_OP query_element */
#line 38 "expr.y"
{
  int mode;
  GRN_UINT32_POP(&efsi->mode_stack, mode);
}
#line 1294 "expr.c"
        break;
      case 10: /* query_element ::= IDENTIFIER RELATIVE_OP query_element */
#line 42 "expr.y"
{
  int mode;
  grn_obj *c;
  GRN_PTR_POP(&efsi->column_stack, c);
  GRN_UINT32_POP(&efsi->mode_stack, mode);
}
#line 1304 "expr.c"
        break;
      case 11: /* query_element ::= BRACEL expression BRACER */
      case 12: /* query_element ::= EVAL primary_expression */
#line 48 "expr.y"
{
  efsi->parse_level = efsi->default_parse_level;
}
#line 1312 "expr.c"
        break;
      case 107: /* arguments ::= PARENL argument_list PARENR */
#line 181 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_CALL, yymsp[-1].minor.yy0);
}
#line 1319 "expr.c"
        break;
      case 108: /* argument_list ::= */
#line 184 "expr.y"
{ yygotominor.yy0 = 0; }
#line 1324 "expr.c"
        break;
      case 109: /* argument_list ::= assignment_expression */
#line 185 "expr.y"
{ yygotominor.yy0 = 1; }
#line 1329 "expr.c"
        break;
      case 110: /* argument_list ::= argument_list COMMA assignment_expression */
#line 186 "expr.y"
{ yygotominor.yy0 = yymsp[-2].minor.yy0 + 1; }
#line 1334 "expr.c"
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
#line 1400 "expr.c"
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
