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
#define YYNSTATE 223
#define YYNRULE 127
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
 /*     0 */   136,    1,  134,   69,   70,   71,   72,   73,   74,   75,
 /*    10 */    76,   77,   91,    3,   66,   61,  137,  351,   65,  123,
 /*    20 */   137,  173,  170,  151,   90,  109,  110,  128,  129,  130,
 /*    30 */   115,   94,  101,  119,  100,  184,  155,  167,   63,   53,
 /*    40 */    54,  174,  178,  135,   50,   51,   52,   55,   56,   57,
 /*    50 */    58,  177,  179,  180,  181,  182,  183,    2,   66,  117,
 /*    60 */   137,  173,  170,  151,   90,  109,  110,  128,  129,  130,
 /*    70 */   115,   94,  101,  119,  100,  184,  155,  167,   63,  189,
 /*    80 */   131,  174,  178,  173,  196,  151,   90,  109,  110,  128,
 /*    90 */   129,  130,  115,   94,  101,  119,  100,  184,  155,  167,
 /*   100 */    63,   27,  138,  174,  178,    8,   88,  108,    9,  173,
 /*   110 */   170,  151,   90,  109,  110,  128,  129,  130,  115,   94,
 /*   120 */   101,  119,  100,  184,  155,  167,   63,   28,   29,  174,
 /*   130 */   178,   12,  108,  120,  173,  170,  151,   90,  109,  110,
 /*   140 */   128,  129,  130,  115,   94,  101,  119,  100,  184,  155,
 /*   150 */   167,   63,   33,   34,  174,  178,  114,  120,  173,  170,
 /*   160 */   151,   90,  109,  110,  128,  129,  130,  115,   94,  101,
 /*   170 */   119,  100,  184,  155,  167,   63,    7,  139,  174,  178,
 /*   180 */   117,  195,  173,  170,  151,   90,  109,  110,  128,  129,
 /*   190 */   130,  115,   94,  101,  119,  100,  184,  155,  167,   63,
 /*   200 */    47,   48,  174,  178,  173,  200,  151,   90,  109,  110,
 /*   210 */   128,  129,  130,  115,   94,  101,  119,  100,  184,  155,
 /*   220 */   167,   63,    6,  194,  174,  178,   13,  191,  191,  140,
 /*   230 */   191,  121,    5,  141,   89,  132,  212,  186,  173,  187,
 /*   240 */   151,   90,  109,  110,  128,  129,  130,  115,   94,  101,
 /*   250 */   119,  100,  184,  155,  167,   63,  174,  178,  174,  178,
 /*   260 */    53,   54,   44,   45,   46,   50,   51,   52,   55,   56,
 /*   270 */    57,   58,  177,  179,  180,  181,  182,  183,    2,  185,
 /*   280 */     6,   78,   79,   80,   81,   82,   83,   84,   86,   85,
 /*   290 */   165,  166,   89,  168,  169,  186,  173,  193,  151,   90,
 /*   300 */   109,  110,  128,  129,  130,  115,   94,  101,  119,  100,
 /*   310 */   184,  155,  167,   63,   10,  142,  174,  178,   53,   54,
 /*   320 */   149,  143,   10,   50,   51,   52,   55,   56,   57,   58,
 /*   330 */   177,  179,  180,  181,  182,  183,    2,  192,  173,  150,
 /*   340 */   151,   90,  109,  110,  128,  129,  130,  115,   94,  101,
 /*   350 */   119,  100,  184,  155,  167,   63,  171,  144,  174,  178,
 /*   360 */   145,  146,   64,  173,  152,  151,   90,  109,  110,  128,
 /*   370 */   129,  130,  115,   94,  101,  119,  100,  184,  155,  167,
 /*   380 */    63,  147,  148,  174,  178,  213,   30,  173,  124,  151,
 /*   390 */    90,  109,  110,  128,  129,  130,  115,   94,  101,  119,
 /*   400 */   100,  184,  155,  167,   63,  188,  214,  174,  178,  173,
 /*   410 */   153,  151,   90,  109,  110,  128,  129,  130,  115,   94,
 /*   420 */   101,  119,  100,  184,  155,  167,   63,  215,  216,  174,
 /*   430 */   178,  217,  218,   31,  173,  190,  151,   90,  109,  110,
 /*   440 */   128,  129,  130,  115,   94,  101,  119,  100,  184,  155,
 /*   450 */   167,   63,  219,  220,  174,  178,  224,   32,  173,  199,
 /*   460 */   151,   90,  109,  110,  128,  129,  130,  115,   94,  101,
 /*   470 */   119,  100,  184,  155,  167,   63,   10,  221,  174,  178,
 /*   480 */   173,  201,  151,   90,  109,  110,  128,  129,  130,  115,
 /*   490 */    94,  101,  119,  100,  184,  155,  167,   63,   14,  197,
 /*   500 */   174,  178,  352,  352,  352,  173,  202,  151,   90,  109,
 /*   510 */   110,  128,  129,  130,  115,   94,  101,  119,  100,  184,
 /*   520 */   155,  167,   63,  352,  352,  174,  178,  352,  352,  173,
 /*   530 */   203,  151,   90,  109,  110,  128,  129,  130,  115,   94,
 /*   540 */   101,  119,  100,  184,  155,  167,   63,  352,  352,  174,
 /*   550 */   178,  173,  204,  151,   90,  109,  110,  128,  129,  130,
 /*   560 */   115,   94,  101,  119,  100,  184,  155,  167,   63,  352,
 /*   570 */   352,  174,  178,  352,  352,  352,  173,  205,  151,   90,
 /*   580 */   109,  110,  128,  129,  130,  115,   94,  101,  119,  100,
 /*   590 */   184,  155,  167,   63,  352,  352,  174,  178,  352,  352,
 /*   600 */   173,  206,  151,   90,  109,  110,  128,  129,  130,  115,
 /*   610 */    94,  101,  119,  100,  184,  155,  167,   63,  352,  352,
 /*   620 */   174,  178,  173,  207,  151,   90,  109,  110,  128,  129,
 /*   630 */   130,  115,   94,  101,  119,  100,  184,  155,  167,   63,
 /*   640 */   352,  352,  174,  178,  352,  352,  352,  173,  208,  151,
 /*   650 */    90,  109,  110,  128,  129,  130,  115,   94,  101,  119,
 /*   660 */   100,  184,  155,  167,   63,  352,  352,  174,  178,  352,
 /*   670 */   352,  173,  209,  151,   90,  109,  110,  128,  129,  130,
 /*   680 */   115,   94,  101,  119,  100,  184,  155,  167,   63,  352,
 /*   690 */   352,  174,  178,  173,  210,  151,   90,  109,  110,  128,
 /*   700 */   129,  130,  115,   94,  101,  119,  100,  184,  155,  167,
 /*   710 */    63,  352,  352,  174,  178,  352,  352,  352,  173,  211,
 /*   720 */   151,   90,  109,  110,  128,  129,  130,  115,   94,  101,
 /*   730 */   119,  100,  184,  155,  167,   63,    6,  352,  174,  178,
 /*   740 */    49,   59,   60,  352,  352,  352,  352,  352,   89,  352,
 /*   750 */   173,  195,  352,  113,  352,  352,  352,    6,  127,  115,
 /*   760 */    94,  101,  119,  100,  184,  155,  167,   63,  352,   89,
 /*   770 */   174,  178,  352,  352,   53,   54,  352,  352,  352,   50,
 /*   780 */    51,   52,   55,   56,   57,   58,  177,  179,  180,  181,
 /*   790 */   182,  183,    2,  352,  352,   53,   54,  352,  352,  352,
 /*   800 */    50,   51,   52,   55,   56,   57,   58,  177,  179,  180,
 /*   810 */   181,  182,  183,    2,  223,   67,   68,   87,  136,   62,
 /*   820 */   352,   69,   70,   71,   72,   73,   74,   75,   76,   77,
 /*   830 */    91,    4,  352,   61,  352,   67,   68,   87,  136,   62,
 /*   840 */   222,   69,   70,   71,   72,   73,   74,   75,   76,   77,
 /*   850 */    91,    4,  173,   61,  352,  113,  175,  122,  128,  129,
 /*   860 */   130,  115,   94,  101,  119,  100,  184,  155,  167,   63,
 /*   870 */    10,  173,  174,  178,  113,  198,  352,  125,  129,  130,
 /*   880 */   115,   94,  101,  119,  100,  184,  155,  167,   63,   15,
 /*   890 */   173,  174,  178,  113,  352,  352,  133,  129,  130,  115,
 /*   900 */    94,  101,  119,  100,  184,  155,  167,   63,  352,  173,
 /*   910 */   174,  178,  113,  352,  352,  352,  126,  130,  115,   94,
 /*   920 */   101,  119,  100,  184,  155,  167,   63,  352,  352,  174,
 /*   930 */   178,  136,   62,  352,   69,   70,   71,   72,   73,   74,
 /*   940 */    75,   76,   77,   91,    4,  352,   61,   11,   16,   17,
 /*   950 */    18,   19,   20,   21,   22,   23,   24,   25,   26,   35,
 /*   960 */    36,   37,   38,   39,   40,   41,   42,   43,  173,  352,
 /*   970 */   352,  113,  352,  352,  352,  165,  166,  111,   94,  101,
 /*   980 */   119,  100,  184,  155,  167,   63,  352,  173,  174,  178,
 /*   990 */   113,  352,  352,  352,  352,  352,  352,   92,  101,  119,
 /*  1000 */   100,  184,  155,  167,   63,  352,  173,  174,  178,  113,
 /*  1010 */   352,  352,  352,  352,  352,  352,   93,  101,  119,  100,
 /*  1020 */   184,  155,  167,   63,  352,  173,  174,  178,  113,  352,
 /*  1030 */   352,  352,  352,  352,  352,  352,   95,  119,  100,  184,
 /*  1040 */   155,  167,   63,  352,  352,  174,  178,  173,  352,  352,
 /*  1050 */   113,  352,  352,  352,  352,  352,  352,  352,   97,  119,
 /*  1060 */   100,  184,  155,  167,   63,  352,  352,  174,  178,  173,
 /*  1070 */   352,  352,  113,  352,  352,  352,  352,  352,  352,  352,
 /*  1080 */    99,  119,  100,  184,  155,  167,   63,  352,  173,  174,
 /*  1090 */   178,  113,  352,  352,  352,  352,  352,  352,  352,  102,
 /*  1100 */   119,  100,  184,  155,  167,   63,  352,  173,  174,  178,
 /*  1110 */   113,  352,  352,  352,  352,  352,  352,  352,  103,  119,
 /*  1120 */   100,  184,  155,  167,   63,  352,  352,  174,  178,  173,
 /*  1130 */   352,  352,  113,  352,  352,  352,  352,  352,  352,  352,
 /*  1140 */   104,  119,  100,  184,  155,  167,   63,  352,  352,  174,
 /*  1150 */   178,  173,  352,  352,  113,  352,  352,  352,  352,  352,
 /*  1160 */   352,  352,  105,  119,  100,  184,  155,  167,   63,  352,
 /*  1170 */   173,  174,  178,  113,  352,  352,  352,  352,  352,  352,
 /*  1180 */   352,  106,  119,  100,  184,  155,  167,   63,  352,  173,
 /*  1190 */   174,  178,  113,  352,  352,  352,  352,  352,  352,  352,
 /*  1200 */   107,  119,  100,  184,  155,  167,   63,    6,  352,  174,
 /*  1210 */   178,  173,  352,  352,  113,  352,  352,  352,  352,   89,
 /*  1220 */   352,  352,  352,  112,  100,  184,  155,  167,   63,  352,
 /*  1230 */   352,  174,  178,  173,  352,  352,  113,  352,  352,  352,
 /*  1240 */   352,  352,  352,  352,  352,  116,  100,  184,  155,  167,
 /*  1250 */    63,  352,  173,  174,  178,  113,  352,  177,  179,  180,
 /*  1260 */   181,  182,  183,    2,  118,  100,  184,  155,  167,   63,
 /*  1270 */   352,  173,  174,  178,  113,  352,  352,  352,  352,  352,
 /*  1280 */   173,  352,  352,  113,   96,  184,  155,  167,   63,  352,
 /*  1290 */   352,  174,  178,   98,  184,  155,  167,   63,  352,  173,
 /*  1300 */   174,  178,  113,  352,  352,  352,  352,  173,  352,  352,
 /*  1310 */   113,  352,  352,  154,  155,  167,   63,  352,  352,  174,
 /*  1320 */   178,  156,  155,  167,   63,  352,  173,  174,  178,  113,
 /*  1330 */   352,  352,  352,  352,  173,  352,  352,  113,  352,  352,
 /*  1340 */   157,  155,  167,   63,  352,  352,  174,  178,  158,  155,
 /*  1350 */   167,   63,  352,  352,  174,  178,  173,  352,  352,  113,
 /*  1360 */   352,  352,  352,  352,  173,  352,  352,  113,  352,  352,
 /*  1370 */   159,  155,  167,   63,  352,  352,  174,  178,  160,  155,
 /*  1380 */   167,   63,  352,  352,  174,  178,  173,  352,  352,  113,
 /*  1390 */   352,  352,  352,  352,  173,  352,  352,  113,  352,  352,
 /*  1400 */   161,  155,  167,   63,  352,  352,  174,  178,  162,  155,
 /*  1410 */   167,   63,  352,  173,  174,  178,  113,  352,  352,  352,
 /*  1420 */   352,  173,  352,  352,  113,  352,  352,  163,  155,  167,
 /*  1430 */    63,  352,  352,  174,  178,  164,  155,  167,   63,  352,
 /*  1440 */   352,  174,  178,  173,  352,  352,  113,  352,  352,  352,
 /*  1450 */   352,  173,  352,  352,  113,  352,  352,  172,  155,  167,
 /*  1460 */    63,  352,  352,  174,  178,  176,  155,  167,   63,  352,
 /*  1470 */   352,  174,  178,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     4,    5,   69,    7,    8,    9,   10,   11,   12,   13,
 /*    10 */    14,   15,   16,   17,   67,   19,   69,   66,   67,   68,
 /*    20 */    69,   70,   71,   72,   73,   74,   75,   76,   77,   78,
 /*    30 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   43,
 /*    40 */    44,   90,   91,   69,   48,   49,   50,   51,   52,   53,
 /*    50 */    54,   55,   56,   57,   58,   59,   60,   61,   67,   68,
 /*    60 */    69,   70,   71,   72,   73,   74,   75,   76,   77,   78,
 /*    70 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   95,
 /*    80 */    96,   90,   91,   70,   71,   72,   73,   74,   75,   76,
 /*    90 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   100 */    87,    3,   69,   90,   91,   92,   93,   68,   92,   70,
 /*   110 */    71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*   120 */    81,   82,   83,   84,   85,   86,   87,    1,    2,   90,
 /*   130 */    91,   33,   68,   94,   70,   71,   72,   73,   74,   75,
 /*   140 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   85,
 /*   150 */    86,   87,   38,   39,   90,   91,   68,   94,   70,   71,
 /*   160 */    72,   73,   74,   75,   76,   77,   78,   79,   80,   81,
 /*   170 */    82,   83,   84,   85,   86,   87,    5,   69,   90,   91,
 /*   180 */    68,   20,   70,   71,   72,   73,   74,   75,   76,   77,
 /*   190 */    78,   79,   80,   81,   82,   83,   84,   85,   86,   87,
 /*   200 */    43,   44,   90,   91,   70,   71,   72,   73,   74,   75,
 /*   210 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   85,
 /*   220 */    86,   87,    5,   62,   90,   91,   34,   55,   56,   69,
 /*   230 */    58,   97,   61,   69,   17,   64,   70,   20,   70,   71,
 /*   240 */    72,   73,   74,   75,   76,   77,   78,   79,   80,   81,
 /*   250 */    82,   83,   84,   85,   86,   87,   90,   91,   90,   91,
 /*   260 */    43,   44,   40,   41,   42,   48,   49,   50,   51,   52,
 /*   270 */    53,   54,   55,   56,   57,   58,   59,   60,   61,   62,
 /*   280 */     5,    7,    8,    9,   10,   11,   12,   13,   14,   15,
 /*   290 */    49,   50,   17,   88,   89,   20,   70,   71,   72,   73,
 /*   300 */    74,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   310 */    84,   85,   86,   87,   20,   69,   90,   91,   43,   44,
 /*   320 */    18,   69,   20,   48,   49,   50,   51,   52,   53,   54,
 /*   330 */    55,   56,   57,   58,   59,   60,   61,   62,   70,   71,
 /*   340 */    72,   73,   74,   75,   76,   77,   78,   79,   80,   81,
 /*   350 */    82,   83,   84,   85,   86,   87,   62,   69,   90,   91,
 /*   360 */    69,   69,   20,   70,   71,   72,   73,   74,   75,   76,
 /*   370 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   380 */    87,   69,   69,   90,   91,   69,   35,   70,   71,   72,
 /*   390 */    73,   74,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   400 */    83,   84,   85,   86,   87,   63,   69,   90,   91,   70,
 /*   410 */    71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*   420 */    81,   82,   83,   84,   85,   86,   87,   69,   69,   90,
 /*   430 */    91,   69,   69,   36,   70,   71,   72,   73,   74,   75,
 /*   440 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   85,
 /*   450 */    86,   87,   69,   69,   90,   91,    0,   37,   70,   71,
 /*   460 */    72,   73,   74,   75,   76,   77,   78,   79,   80,   81,
 /*   470 */    82,   83,   84,   85,   86,   87,   20,   69,   90,   91,
 /*   480 */    70,   71,   72,   73,   74,   75,   76,   77,   78,   79,
 /*   490 */    80,   81,   82,   83,   84,   85,   86,   87,   34,   55,
 /*   500 */    90,   91,   98,   98,   98,   70,   71,   72,   73,   74,
 /*   510 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   520 */    85,   86,   87,   98,   98,   90,   91,   98,   98,   70,
 /*   530 */    71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*   540 */    81,   82,   83,   84,   85,   86,   87,   98,   98,   90,
 /*   550 */    91,   70,   71,   72,   73,   74,   75,   76,   77,   78,
 /*   560 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   98,
 /*   570 */    98,   90,   91,   98,   98,   98,   70,   71,   72,   73,
 /*   580 */    74,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   590 */    84,   85,   86,   87,   98,   98,   90,   91,   98,   98,
 /*   600 */    70,   71,   72,   73,   74,   75,   76,   77,   78,   79,
 /*   610 */    80,   81,   82,   83,   84,   85,   86,   87,   98,   98,
 /*   620 */    90,   91,   70,   71,   72,   73,   74,   75,   76,   77,
 /*   630 */    78,   79,   80,   81,   82,   83,   84,   85,   86,   87,
 /*   640 */    98,   98,   90,   91,   98,   98,   98,   70,   71,   72,
 /*   650 */    73,   74,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   660 */    83,   84,   85,   86,   87,   98,   98,   90,   91,   98,
 /*   670 */    98,   70,   71,   72,   73,   74,   75,   76,   77,   78,
 /*   680 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   98,
 /*   690 */    98,   90,   91,   70,   71,   72,   73,   74,   75,   76,
 /*   700 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   710 */    87,   98,   98,   90,   91,   98,   98,   98,   70,   71,
 /*   720 */    72,   73,   74,   75,   76,   77,   78,   79,   80,   81,
 /*   730 */    82,   83,   84,   85,   86,   87,    5,   98,   90,   91,
 /*   740 */    45,   46,   47,   98,   98,   98,   98,   98,   17,   98,
 /*   750 */    70,   20,   98,   73,   98,   98,   98,    5,   78,   79,
 /*   760 */    80,   81,   82,   83,   84,   85,   86,   87,   98,   17,
 /*   770 */    90,   91,   98,   98,   43,   44,   98,   98,   98,   48,
 /*   780 */    49,   50,   51,   52,   53,   54,   55,   56,   57,   58,
 /*   790 */    59,   60,   61,   98,   98,   43,   44,   98,   98,   98,
 /*   800 */    48,   49,   50,   51,   52,   53,   54,   55,   56,   57,
 /*   810 */    58,   59,   60,   61,    0,    1,    2,    3,    4,    5,
 /*   820 */    98,    7,    8,    9,   10,   11,   12,   13,   14,   15,
 /*   830 */    16,   17,   98,   19,   98,    1,    2,    3,    4,    5,
 /*   840 */     6,    7,    8,    9,   10,   11,   12,   13,   14,   15,
 /*   850 */    16,   17,   70,   19,   98,   73,    6,   75,   76,   77,
 /*   860 */    78,   79,   80,   81,   82,   83,   84,   85,   86,   87,
 /*   870 */    20,   70,   90,   91,   73,    6,   98,   76,   77,   78,
 /*   880 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   20,
 /*   890 */    70,   90,   91,   73,   98,   98,   76,   77,   78,   79,
 /*   900 */    80,   81,   82,   83,   84,   85,   86,   87,   98,   70,
 /*   910 */    90,   91,   73,   98,   98,   98,   77,   78,   79,   80,
 /*   920 */    81,   82,   83,   84,   85,   86,   87,   98,   98,   90,
 /*   930 */    91,    4,    5,   98,    7,    8,    9,   10,   11,   12,
 /*   940 */    13,   14,   15,   16,   17,   98,   19,   21,   22,   23,
 /*   950 */    24,   25,   26,   27,   28,   29,   30,   31,   32,    7,
 /*   960 */     8,    9,   10,   11,   12,   13,   14,   15,   70,   98,
 /*   970 */    98,   73,   98,   98,   98,   49,   50,   79,   80,   81,
 /*   980 */    82,   83,   84,   85,   86,   87,   98,   70,   90,   91,
 /*   990 */    73,   98,   98,   98,   98,   98,   98,   80,   81,   82,
 /*  1000 */    83,   84,   85,   86,   87,   98,   70,   90,   91,   73,
 /*  1010 */    98,   98,   98,   98,   98,   98,   80,   81,   82,   83,
 /*  1020 */    84,   85,   86,   87,   98,   70,   90,   91,   73,   98,
 /*  1030 */    98,   98,   98,   98,   98,   98,   81,   82,   83,   84,
 /*  1040 */    85,   86,   87,   98,   98,   90,   91,   70,   98,   98,
 /*  1050 */    73,   98,   98,   98,   98,   98,   98,   98,   81,   82,
 /*  1060 */    83,   84,   85,   86,   87,   98,   98,   90,   91,   70,
 /*  1070 */    98,   98,   73,   98,   98,   98,   98,   98,   98,   98,
 /*  1080 */    81,   82,   83,   84,   85,   86,   87,   98,   70,   90,
 /*  1090 */    91,   73,   98,   98,   98,   98,   98,   98,   98,   81,
 /*  1100 */    82,   83,   84,   85,   86,   87,   98,   70,   90,   91,
 /*  1110 */    73,   98,   98,   98,   98,   98,   98,   98,   81,   82,
 /*  1120 */    83,   84,   85,   86,   87,   98,   98,   90,   91,   70,
 /*  1130 */    98,   98,   73,   98,   98,   98,   98,   98,   98,   98,
 /*  1140 */    81,   82,   83,   84,   85,   86,   87,   98,   98,   90,
 /*  1150 */    91,   70,   98,   98,   73,   98,   98,   98,   98,   98,
 /*  1160 */    98,   98,   81,   82,   83,   84,   85,   86,   87,   98,
 /*  1170 */    70,   90,   91,   73,   98,   98,   98,   98,   98,   98,
 /*  1180 */    98,   81,   82,   83,   84,   85,   86,   87,   98,   70,
 /*  1190 */    90,   91,   73,   98,   98,   98,   98,   98,   98,   98,
 /*  1200 */    81,   82,   83,   84,   85,   86,   87,    5,   98,   90,
 /*  1210 */    91,   70,   98,   98,   73,   98,   98,   98,   98,   17,
 /*  1220 */    98,   98,   98,   82,   83,   84,   85,   86,   87,   98,
 /*  1230 */    98,   90,   91,   70,   98,   98,   73,   98,   98,   98,
 /*  1240 */    98,   98,   98,   98,   98,   82,   83,   84,   85,   86,
 /*  1250 */    87,   98,   70,   90,   91,   73,   98,   55,   56,   57,
 /*  1260 */    58,   59,   60,   61,   82,   83,   84,   85,   86,   87,
 /*  1270 */    98,   70,   90,   91,   73,   98,   98,   98,   98,   98,
 /*  1280 */    70,   98,   98,   73,   83,   84,   85,   86,   87,   98,
 /*  1290 */    98,   90,   91,   83,   84,   85,   86,   87,   98,   70,
 /*  1300 */    90,   91,   73,   98,   98,   98,   98,   70,   98,   98,
 /*  1310 */    73,   98,   98,   84,   85,   86,   87,   98,   98,   90,
 /*  1320 */    91,   84,   85,   86,   87,   98,   70,   90,   91,   73,
 /*  1330 */    98,   98,   98,   98,   70,   98,   98,   73,   98,   98,
 /*  1340 */    84,   85,   86,   87,   98,   98,   90,   91,   84,   85,
 /*  1350 */    86,   87,   98,   98,   90,   91,   70,   98,   98,   73,
 /*  1360 */    98,   98,   98,   98,   70,   98,   98,   73,   98,   98,
 /*  1370 */    84,   85,   86,   87,   98,   98,   90,   91,   84,   85,
 /*  1380 */    86,   87,   98,   98,   90,   91,   70,   98,   98,   73,
 /*  1390 */    98,   98,   98,   98,   70,   98,   98,   73,   98,   98,
 /*  1400 */    84,   85,   86,   87,   98,   98,   90,   91,   84,   85,
 /*  1410 */    86,   87,   98,   70,   90,   91,   73,   98,   98,   98,
 /*  1420 */    98,   70,   98,   98,   73,   98,   98,   84,   85,   86,
 /*  1430 */    87,   98,   98,   90,   91,   84,   85,   86,   87,   98,
 /*  1440 */    98,   90,   91,   70,   98,   98,   73,   98,   98,   98,
 /*  1450 */    98,   70,   98,   98,   73,   98,   98,   84,   85,   86,
 /*  1460 */    87,   98,   98,   90,   91,   84,   85,   86,   87,   98,
 /*  1470 */    98,   90,   91,
};
#define YY_SHIFT_USE_DFLT (-5)
#define YY_SHIFT_MAX 133
static const short yy_shift_ofst[] = {
 /*     0 */    -4,   -4,  731,  752,  752,  752,  752,  752,  217,  275,
 /*    10 */   752,  752,  752,  752,  752,  752,  752,  752,  752,  752,
 /*    20 */   752,  752,  752,  752,  752,  752,  752,  752,  752,  752,
 /*    30 */   752,  752,  752,  752,  752,  752,  752,  752,  752,  752,
 /*    40 */   752,  752,  752,  752,  752,  752,  752,  752,  752,  752,
 /*    50 */   752,  752,  752,  752,  752,  752,  752,  752,  752,  752,
 /*    60 */   752, 1202,  927,  171,  172,  814,  834,  927,  927,  927,
 /*    70 */   927,  927,  927,  927,  927,  927,  927,  927,  927,  927,
 /*    80 */   927,  927,  927,  927,  927,  927,  927,  927,  161,   -5,
 /*    90 */   926,  274,  952,  952,  952,  222,  695,  222,  695,  222,
 /*   100 */   695,  222,  222,  222,  222,  222,  222,  222,  302,   98,
 /*   110 */   126,  114,  157,  241,  294,  114,  157,  850,  157,  157,
 /*   120 */   342,  869,  126,  456,  192,  351,  397,  420,  351,  397,
 /*   130 */   420,  464,  444,  351,
};
#define YY_REDUCE_USE_DFLT (-68)
#define YY_REDUCE_MAX 89
static const short yy_reduce_ofst[] = {
 /*     0 */   -49,   -9,   13,   39,   64,   88,  112,  134,  168,  226,
 /*    10 */   268,  293,  317,  339,  364,  388,  410,  435,  459,  481,
 /*    20 */   506,  530,  552,  577,  601,  623,  648,  782,  801,  820,
 /*    30 */   839,  680,  898,  917,  936,  955,  977,  999, 1018, 1037,
 /*    40 */  1059, 1081, 1100, 1119, 1141, 1163, 1182, 1201, 1210, 1229,
 /*    50 */  1237, 1256, 1264, 1286, 1294, 1316, 1324, 1343, 1351, 1373,
 /*    60 */  1381,  166,  -53,  205,  -16,  -67,  -67,  -26,   33,  108,
 /*    70 */   160,  164,  246,  252,  288,  291,  292,  312,  313,  316,
 /*    80 */   337,  358,  359,  362,  363,  383,  384,  408,   16,   63,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   350,  350,  350,  340,  350,  350,  350,  347,  350,  350,
 /*    10 */   350,  350,  350,  350,  350,  350,  350,  350,  350,  350,
 /*    20 */   350,  350,  350,  350,  350,  350,  350,  350,  350,  350,
 /*    30 */   350,  350,  350,  350,  350,  350,  350,  350,  350,  350,
 /*    40 */   350,  350,  350,  350,  350,  350,  350,  350,  350,  350,
 /*    50 */   350,  350,  350,  350,  350,  350,  350,  350,  350,  350,
 /*    60 */   350,  350,  350,  318,  350,  350,  350,  350,  350,  350,
 /*    70 */   350,  350,  350,  350,  350,  350,  350,  350,  350,  350,
 /*    80 */   350,  350,  350,  350,  350,  350,  350,  350,  350,  340,
 /*    90 */   314,  350,  281,  282,  280,  284,  298,  285,  299,  286,
 /*   100 */   297,  283,  287,  288,  289,  290,  291,  292,  350,  267,
 /*   110 */   269,  279,  294,  314,  350,  278,  295,  350,  296,  293,
 /*   120 */   350,  350,  270,  350,  350,  272,  275,  277,  271,  274,
 /*   130 */   276,  350,  350,  273,  226,  227,  230,  225,  228,  232,
 /*   140 */   233,  234,  235,  236,  237,  238,  239,  240,  241,  250,
 /*   150 */   253,  254,  255,  268,  301,  304,  305,  306,  307,  308,
 /*   160 */   309,  310,  311,  312,  313,  315,  316,  317,  319,  321,
 /*   170 */   252,  344,  302,  320,  322,  323,  303,  324,  325,  326,
 /*   180 */   327,  328,  329,  330,  300,  331,  335,  337,  339,  341,
 /*   190 */   342,  343,  332,  338,  333,  334,  336,  345,  346,  349,
 /*   200 */   348,  256,  257,  258,  259,  260,  261,  262,  263,  264,
 /*   210 */   265,  266,  251,  242,  243,  244,  245,  246,  247,  248,
 /*   220 */   249,  229,  231,
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
  "QSTRING",       "PARENL",        "PARENR",        "LESS",        
  "GREATER",       "LESS_EQUAL",    "GREATER_EQUAL",  "IN",          
  "MATCH",         "NEAR",          "SIMILAR",       "EXTRACT",     
  "COLUMN",        "BRACEL",        "BRACER",        "EVAL",        
  "COMMA",         "ASSIGN",        "STAR_ASSIGN",   "SLASH_ASSIGN",
  "MOD_ASSIGN",    "PLUS_ASSIGN",   "MINUS_ASSIGN",  "SHIFTL_ASSIGN",
  "SHIRTR_ASSIGN",  "SHIFTRR_ASSIGN",  "AND_ASSIGN",    "XOR_ASSIGN",  
  "OR_ASSIGN",     "QUESTION",      "COLON",         "BITWISE_OR",  
  "BITWISE_XOR",   "BITWISE_AND",   "EQUAL",         "NOT_EQUAL",   
  "SHIFTL",        "SHIFTR",        "SHIFTRR",       "PLUS",        
  "MINUS",         "STAR",          "SLASH",         "MOD",         
  "DELETE",        "INCR",          "DECR",          "NOT",         
  "ADJ_INC",       "ADJ_DEC",       "ADJ_NEG",       "IDENTIFIER",  
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
 /*   9 */ "query_element ::= LESS query_element",
 /*  10 */ "query_element ::= GREATER query_element",
 /*  11 */ "query_element ::= LESS_EQUAL query_element",
 /*  12 */ "query_element ::= GREATER_EQUAL query_element",
 /*  13 */ "query_element ::= IN query_element",
 /*  14 */ "query_element ::= MATCH query_element",
 /*  15 */ "query_element ::= NEAR query_element",
 /*  16 */ "query_element ::= SIMILAR query_element",
 /*  17 */ "query_element ::= EXTRACT query_element",
 /*  18 */ "query_element ::= COLUMN LESS query_element",
 /*  19 */ "query_element ::= COLUMN GREATER query_element",
 /*  20 */ "query_element ::= COLUMN LESS_EQUAL query_element",
 /*  21 */ "query_element ::= COLUMN GREATER_EQUAL query_element",
 /*  22 */ "query_element ::= COLUMN IN query_element",
 /*  23 */ "query_element ::= COLUMN MATCH query_element",
 /*  24 */ "query_element ::= COLUMN NEAR query_element",
 /*  25 */ "query_element ::= COLUMN EXTRACT query_element",
 /*  26 */ "query_element ::= COLUMN SIMILAR query_element",
 /*  27 */ "query_element ::= BRACEL expression BRACER",
 /*  28 */ "query_element ::= EVAL primary_expression",
 /*  29 */ "expression ::= assignment_expression",
 /*  30 */ "expression ::= expression COMMA assignment_expression",
 /*  31 */ "assignment_expression ::= conditional_expression",
 /*  32 */ "assignment_expression ::= lefthand_side_expression ASSIGN assignment_expression",
 /*  33 */ "assignment_expression ::= lefthand_side_expression STAR_ASSIGN assignment_expression",
 /*  34 */ "assignment_expression ::= lefthand_side_expression SLASH_ASSIGN assignment_expression",
 /*  35 */ "assignment_expression ::= lefthand_side_expression MOD_ASSIGN assignment_expression",
 /*  36 */ "assignment_expression ::= lefthand_side_expression PLUS_ASSIGN assignment_expression",
 /*  37 */ "assignment_expression ::= lefthand_side_expression MINUS_ASSIGN assignment_expression",
 /*  38 */ "assignment_expression ::= lefthand_side_expression SHIFTL_ASSIGN assignment_expression",
 /*  39 */ "assignment_expression ::= lefthand_side_expression SHIRTR_ASSIGN assignment_expression",
 /*  40 */ "assignment_expression ::= lefthand_side_expression SHIFTRR_ASSIGN assignment_expression",
 /*  41 */ "assignment_expression ::= lefthand_side_expression AND_ASSIGN assignment_expression",
 /*  42 */ "assignment_expression ::= lefthand_side_expression XOR_ASSIGN assignment_expression",
 /*  43 */ "assignment_expression ::= lefthand_side_expression OR_ASSIGN assignment_expression",
 /*  44 */ "conditional_expression ::= logical_or_expression",
 /*  45 */ "conditional_expression ::= logical_or_expression QUESTION assignment_expression COLON assignment_expression",
 /*  46 */ "logical_or_expression ::= logical_and_expression",
 /*  47 */ "logical_or_expression ::= logical_or_expression LOGICAL_OR logical_and_expression",
 /*  48 */ "logical_and_expression ::= bitwise_or_expression",
 /*  49 */ "logical_and_expression ::= logical_and_expression LOGICAL_AND bitwise_or_expression",
 /*  50 */ "logical_and_expression ::= logical_and_expression LOGICAL_BUT bitwise_or_expression",
 /*  51 */ "bitwise_or_expression ::= bitwise_xor_expression",
 /*  52 */ "bitwise_or_expression ::= bitwise_or_expression BITWISE_OR bitwise_xor_expression",
 /*  53 */ "bitwise_xor_expression ::= bitwise_and_expression",
 /*  54 */ "bitwise_xor_expression ::= bitwise_xor_expression BITWISE_XOR bitwise_and_expression",
 /*  55 */ "bitwise_and_expression ::= equality_expression",
 /*  56 */ "bitwise_and_expression ::= bitwise_and_expression BITWISE_AND equality_expression",
 /*  57 */ "equality_expression ::= relational_expression",
 /*  58 */ "equality_expression ::= equality_expression EQUAL relational_expression",
 /*  59 */ "equality_expression ::= equality_expression NOT_EQUAL relational_expression",
 /*  60 */ "relational_expression ::= shift_expression",
 /*  61 */ "relational_expression ::= relational_expression LESS shift_expression",
 /*  62 */ "relational_expression ::= relational_expression GREATER shift_expression",
 /*  63 */ "relational_expression ::= relational_expression LESS_EQUAL shift_expression",
 /*  64 */ "relational_expression ::= relational_expression GREATER_EQUAL shift_expression",
 /*  65 */ "relational_expression ::= relational_expression IN shift_expression",
 /*  66 */ "relational_expression ::= relational_expression MATCH shift_expression",
 /*  67 */ "relational_expression ::= relational_expression NEAR shift_expression",
 /*  68 */ "relational_expression ::= relational_expression SIMILAR shift_expression",
 /*  69 */ "relational_expression ::= relational_expression EXTRACT shift_expression",
 /*  70 */ "shift_expression ::= additive_expression",
 /*  71 */ "shift_expression ::= shift_expression SHIFTL additive_expression",
 /*  72 */ "shift_expression ::= shift_expression SHIFTR additive_expression",
 /*  73 */ "shift_expression ::= shift_expression SHIFTRR additive_expression",
 /*  74 */ "additive_expression ::= multiplicative_expression",
 /*  75 */ "additive_expression ::= additive_expression PLUS multiplicative_expression",
 /*  76 */ "additive_expression ::= additive_expression MINUS multiplicative_expression",
 /*  77 */ "multiplicative_expression ::= unary_expression",
 /*  78 */ "multiplicative_expression ::= multiplicative_expression STAR unary_expression",
 /*  79 */ "multiplicative_expression ::= multiplicative_expression SLASH unary_expression",
 /*  80 */ "multiplicative_expression ::= multiplicative_expression MOD unary_expression",
 /*  81 */ "unary_expression ::= postfix_expression",
 /*  82 */ "unary_expression ::= DELETE unary_expression",
 /*  83 */ "unary_expression ::= INCR unary_expression",
 /*  84 */ "unary_expression ::= DECR unary_expression",
 /*  85 */ "unary_expression ::= PLUS unary_expression",
 /*  86 */ "unary_expression ::= MINUS unary_expression",
 /*  87 */ "unary_expression ::= NOT unary_expression",
 /*  88 */ "unary_expression ::= ADJ_INC unary_expression",
 /*  89 */ "unary_expression ::= ADJ_DEC unary_expression",
 /*  90 */ "unary_expression ::= ADJ_NEG unary_expression",
 /*  91 */ "postfix_expression ::= lefthand_side_expression",
 /*  92 */ "postfix_expression ::= lefthand_side_expression INCR",
 /*  93 */ "postfix_expression ::= lefthand_side_expression DECR",
 /*  94 */ "lefthand_side_expression ::= call_expression",
 /*  95 */ "lefthand_side_expression ::= member_expression",
 /*  96 */ "call_expression ::= member_expression arguments",
 /*  97 */ "member_expression ::= primary_expression",
 /*  98 */ "member_expression ::= member_expression member_expression_part",
 /*  99 */ "primary_expression ::= object_literal",
 /* 100 */ "primary_expression ::= PARENL expression PARENR",
 /* 101 */ "primary_expression ::= IDENTIFIER",
 /* 102 */ "primary_expression ::= array_literal",
 /* 103 */ "primary_expression ::= DECIMAL",
 /* 104 */ "primary_expression ::= HEX_INTEGER",
 /* 105 */ "primary_expression ::= STRING",
 /* 106 */ "primary_expression ::= BOOLEAN",
 /* 107 */ "primary_expression ::= NULL",
 /* 108 */ "array_literal ::= BRACKETL elision BRACKETR",
 /* 109 */ "array_literal ::= BRACKETL element_list elision BRACKETR",
 /* 110 */ "array_literal ::= BRACKETL element_list BRACKETR",
 /* 111 */ "elision ::= COMMA",
 /* 112 */ "elision ::= elision COMMA",
 /* 113 */ "element_list ::= assignment_expression",
 /* 114 */ "element_list ::= elision assignment_expression",
 /* 115 */ "element_list ::= element_list elision assignment_expression",
 /* 116 */ "object_literal ::= BRACEL property_name_and_value_list RBRACE",
 /* 117 */ "property_name_and_value_list ::=",
 /* 118 */ "property_name_and_value_list ::= property_name_and_value_list COMMA property_name_and_value",
 /* 119 */ "property_name_and_value ::= property_name COLON assignment_expression",
 /* 120 */ "property_name ::= IDENTIFIER|STRING|DECIMAL",
 /* 121 */ "member_expression_part ::= BRACKETL expression BRACKETR",
 /* 122 */ "member_expression_part ::= DOT IDENTIFIER",
 /* 123 */ "arguments ::= PARENL argument_list PARENR",
 /* 124 */ "argument_list ::=",
 /* 125 */ "argument_list ::= assignment_expression",
 /* 126 */ "argument_list ::= argument_list COMMA assignment_expression",
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
  { 69, 2 },
  { 69, 2 },
  { 69, 2 },
  { 69, 2 },
  { 69, 2 },
  { 69, 2 },
  { 69, 2 },
  { 69, 2 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
  { 69, 3 },
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
      case 9: /* query_element ::= LESS query_element */
      case 10: /* query_element ::= GREATER query_element */
      case 11: /* query_element ::= LESS_EQUAL query_element */
      case 12: /* query_element ::= GREATER_EQUAL query_element */
      case 13: /* query_element ::= IN query_element */
      case 14: /* query_element ::= MATCH query_element */
      case 15: /* query_element ::= NEAR query_element */
      case 16: /* query_element ::= SIMILAR query_element */
      case 17: /* query_element ::= EXTRACT query_element */
      case 18: /* query_element ::= COLUMN LESS query_element */
      case 19: /* query_element ::= COLUMN GREATER query_element */
      case 20: /* query_element ::= COLUMN LESS_EQUAL query_element */
      case 21: /* query_element ::= COLUMN GREATER_EQUAL query_element */
      case 22: /* query_element ::= COLUMN IN query_element */
      case 23: /* query_element ::= COLUMN MATCH query_element */
      case 24: /* query_element ::= COLUMN NEAR query_element */
      case 25: /* query_element ::= COLUMN EXTRACT query_element */
      case 26: /* query_element ::= COLUMN SIMILAR query_element */
      case 29: /* expression ::= assignment_expression */
      case 30: /* expression ::= expression COMMA assignment_expression */
      case 31: /* assignment_expression ::= conditional_expression */
      case 32: /* assignment_expression ::= lefthand_side_expression ASSIGN assignment_expression */
      case 33: /* assignment_expression ::= lefthand_side_expression STAR_ASSIGN assignment_expression */
      case 34: /* assignment_expression ::= lefthand_side_expression SLASH_ASSIGN assignment_expression */
      case 35: /* assignment_expression ::= lefthand_side_expression MOD_ASSIGN assignment_expression */
      case 36: /* assignment_expression ::= lefthand_side_expression PLUS_ASSIGN assignment_expression */
      case 37: /* assignment_expression ::= lefthand_side_expression MINUS_ASSIGN assignment_expression */
      case 38: /* assignment_expression ::= lefthand_side_expression SHIFTL_ASSIGN assignment_expression */
      case 39: /* assignment_expression ::= lefthand_side_expression SHIRTR_ASSIGN assignment_expression */
      case 40: /* assignment_expression ::= lefthand_side_expression SHIFTRR_ASSIGN assignment_expression */
      case 41: /* assignment_expression ::= lefthand_side_expression AND_ASSIGN assignment_expression */
      case 42: /* assignment_expression ::= lefthand_side_expression XOR_ASSIGN assignment_expression */
      case 43: /* assignment_expression ::= lefthand_side_expression OR_ASSIGN assignment_expression */
      case 44: /* conditional_expression ::= logical_or_expression */
      case 45: /* conditional_expression ::= logical_or_expression QUESTION assignment_expression COLON assignment_expression */
      case 46: /* logical_or_expression ::= logical_and_expression */
      case 48: /* logical_and_expression ::= bitwise_or_expression */
      case 51: /* bitwise_or_expression ::= bitwise_xor_expression */
      case 52: /* bitwise_or_expression ::= bitwise_or_expression BITWISE_OR bitwise_xor_expression */
      case 53: /* bitwise_xor_expression ::= bitwise_and_expression */
      case 54: /* bitwise_xor_expression ::= bitwise_xor_expression BITWISE_XOR bitwise_and_expression */
      case 55: /* bitwise_and_expression ::= equality_expression */
      case 56: /* bitwise_and_expression ::= bitwise_and_expression BITWISE_AND equality_expression */
      case 57: /* equality_expression ::= relational_expression */
      case 58: /* equality_expression ::= equality_expression EQUAL relational_expression */
      case 59: /* equality_expression ::= equality_expression NOT_EQUAL relational_expression */
      case 60: /* relational_expression ::= shift_expression */
      case 61: /* relational_expression ::= relational_expression LESS shift_expression */
      case 62: /* relational_expression ::= relational_expression GREATER shift_expression */
      case 63: /* relational_expression ::= relational_expression LESS_EQUAL shift_expression */
      case 64: /* relational_expression ::= relational_expression GREATER_EQUAL shift_expression */
      case 65: /* relational_expression ::= relational_expression IN shift_expression */
      case 66: /* relational_expression ::= relational_expression MATCH shift_expression */
      case 67: /* relational_expression ::= relational_expression NEAR shift_expression */
      case 68: /* relational_expression ::= relational_expression SIMILAR shift_expression */
      case 69: /* relational_expression ::= relational_expression EXTRACT shift_expression */
      case 70: /* shift_expression ::= additive_expression */
      case 71: /* shift_expression ::= shift_expression SHIFTL additive_expression */
      case 72: /* shift_expression ::= shift_expression SHIFTR additive_expression */
      case 73: /* shift_expression ::= shift_expression SHIFTRR additive_expression */
      case 74: /* additive_expression ::= multiplicative_expression */
      case 75: /* additive_expression ::= additive_expression PLUS multiplicative_expression */
      case 76: /* additive_expression ::= additive_expression MINUS multiplicative_expression */
      case 77: /* multiplicative_expression ::= unary_expression */
      case 78: /* multiplicative_expression ::= multiplicative_expression STAR unary_expression */
      case 79: /* multiplicative_expression ::= multiplicative_expression SLASH unary_expression */
      case 80: /* multiplicative_expression ::= multiplicative_expression MOD unary_expression */
      case 81: /* unary_expression ::= postfix_expression */
      case 82: /* unary_expression ::= DELETE unary_expression */
      case 83: /* unary_expression ::= INCR unary_expression */
      case 84: /* unary_expression ::= DECR unary_expression */
      case 85: /* unary_expression ::= PLUS unary_expression */
      case 86: /* unary_expression ::= MINUS unary_expression */
      case 87: /* unary_expression ::= NOT unary_expression */
      case 88: /* unary_expression ::= ADJ_INC unary_expression */
      case 89: /* unary_expression ::= ADJ_DEC unary_expression */
      case 90: /* unary_expression ::= ADJ_NEG unary_expression */
      case 91: /* postfix_expression ::= lefthand_side_expression */
      case 92: /* postfix_expression ::= lefthand_side_expression INCR */
      case 93: /* postfix_expression ::= lefthand_side_expression DECR */
      case 94: /* lefthand_side_expression ::= call_expression */
      case 95: /* lefthand_side_expression ::= member_expression */
      case 96: /* call_expression ::= member_expression arguments */
      case 97: /* member_expression ::= primary_expression */
      case 98: /* member_expression ::= member_expression member_expression_part */
      case 99: /* primary_expression ::= object_literal */
      case 100: /* primary_expression ::= PARENL expression PARENR */
      case 101: /* primary_expression ::= IDENTIFIER */
      case 102: /* primary_expression ::= array_literal */
      case 103: /* primary_expression ::= DECIMAL */
      case 104: /* primary_expression ::= HEX_INTEGER */
      case 105: /* primary_expression ::= STRING */
      case 106: /* primary_expression ::= BOOLEAN */
      case 107: /* primary_expression ::= NULL */
      case 108: /* array_literal ::= BRACKETL elision BRACKETR */
      case 109: /* array_literal ::= BRACKETL element_list elision BRACKETR */
      case 110: /* array_literal ::= BRACKETL element_list BRACKETR */
      case 111: /* elision ::= COMMA */
      case 112: /* elision ::= elision COMMA */
      case 113: /* element_list ::= assignment_expression */
      case 114: /* element_list ::= elision assignment_expression */
      case 115: /* element_list ::= element_list elision assignment_expression */
      case 116: /* object_literal ::= BRACEL property_name_and_value_list RBRACE */
      case 117: /* property_name_and_value_list ::= */
      case 118: /* property_name_and_value_list ::= property_name_and_value_list COMMA property_name_and_value */
      case 119: /* property_name_and_value ::= property_name COLON assignment_expression */
      case 120: /* property_name ::= IDENTIFIER|STRING|DECIMAL */
      case 121: /* member_expression_part ::= BRACKETL expression BRACKETR */
      case 122: /* member_expression_part ::= DOT IDENTIFIER */
#line 18 "expr.y"
{
}
#line 1315 "expr.c"
        break;
      case 3: /* query ::= query query_element */
#line 22 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, grn_int32_value_at(&efsi->op_stack, -1), 2);
}
#line 1322 "expr.c"
        break;
      case 4: /* query ::= query LOGICAL_AND query_element */
      case 49: /* logical_and_expression ::= logical_and_expression LOGICAL_AND bitwise_or_expression */
#line 25 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND, 2);
}
#line 1330 "expr.c"
        break;
      case 5: /* query ::= query LOGICAL_BUT query_element */
      case 50: /* logical_and_expression ::= logical_and_expression LOGICAL_BUT bitwise_or_expression */
#line 28 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BUT, 2);
}
#line 1338 "expr.c"
        break;
      case 6: /* query ::= query LOGICAL_OR query_element */
      case 47: /* logical_or_expression ::= logical_or_expression LOGICAL_OR logical_and_expression */
#line 31 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_OR, 2);
}
#line 1346 "expr.c"
        break;
      case 27: /* query_element ::= BRACEL expression BRACER */
      case 28: /* query_element ::= EVAL primary_expression */
#line 57 "expr.y"
{
  efsi->parse_level = efsi->default_parse_level;
}
#line 1354 "expr.c"
        break;
      case 123: /* arguments ::= PARENL argument_list PARENR */
#line 190 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_CALL, yymsp[-1].minor.yy0);
}
#line 1361 "expr.c"
        break;
      case 124: /* argument_list ::= */
#line 193 "expr.y"
{ yygotominor.yy0 = 0; }
#line 1366 "expr.c"
        break;
      case 125: /* argument_list ::= assignment_expression */
#line 194 "expr.y"
{ yygotominor.yy0 = 1; }
#line 1371 "expr.c"
        break;
      case 126: /* argument_list ::= argument_list COMMA assignment_expression */
#line 195 "expr.y"
{ yygotominor.yy0 = yymsp[-2].minor.yy0 + 1; }
#line 1376 "expr.c"
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
#line 1442 "expr.c"
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
