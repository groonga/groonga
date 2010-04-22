/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is included that follows the "include" declaration
** in the input grammar file. */
#include <stdio.h>
#line 3 "expr.y"

#define assert GRN_ASSERT
#line 11 "expr.c"
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
#define YYNOCODE 104
#define YYACTIONTYPE unsigned short int
#define grn_expr_parserTOKENTYPE  int 
typedef union {
  int yyinit;
  grn_expr_parserTOKENTYPE yy0;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define grn_expr_parserARG_SDECL  efs_info *efsi ;
#define grn_expr_parserARG_PDECL , efs_info *efsi 
#define grn_expr_parserARG_FETCH  efs_info *efsi  = yypParser->efsi 
#define grn_expr_parserARG_STORE yypParser->efsi  = efsi 
#define YYNSTATE 204
#define YYNRULE 117
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* The yyzerominor constant is used to initialize instances of
** YYMINORTYPE objects to zero. */
static const YYMINORTYPE yyzerominor = { 0 };

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
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
#define YY_ACTTAB_COUNT (1583)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   322,   72,  101,  200,  170,  173,  194,   80,  115,  114,
 /*    10 */   123,  122,  121,  109,   81,   94,  105,   95,  159,  190,
 /*    20 */   176,   70,   29,   28,  169,  165,   71,  107,  200,  170,
 /*    30 */   173,  194,   80,  115,  114,  123,  122,  121,  109,   81,
 /*    40 */    94,  105,   95,  159,  190,  176,   70,   34,   33,  169,
 /*    50 */   165,  201,    1,   74,   75,  117,    3,  116,   67,  170,
 /*    60 */   173,  194,   80,  115,  114,  123,  122,  121,  109,   81,
 /*    70 */    94,  105,   95,  159,  190,  176,   70,   52,   51,  169,
 /*    80 */   165,  201,   68,  104,   75,  128,    7,    4,   67,   50,
 /*    90 */    49,   48,   66,   54,   53,   62,   61,  196,  152,   26,
 /*   100 */    65,   64,   63,   60,   59,   58,   57,   56,   55,  164,
 /*   110 */   163,  162,  161,  160,    2,    5,  178,  177,  166,   79,
 /*   120 */    23,  155,  157,   69,   30,  170,  147,  194,   80,  115,
 /*   130 */   114,  123,  122,  121,  109,   81,   94,  105,   95,  159,
 /*   140 */   190,  176,   70,  154,  120,  169,  165,    9,   78,    6,
 /*   150 */    26,  119,  152,   31,  152,  168,  175,  174,   62,   61,
 /*   160 */   131,   26,   27,   65,   64,   63,   60,   59,   58,   57,
 /*   170 */    56,   55,  164,  163,  162,  161,  160,    2,  158,    5,
 /*   180 */   169,  165,  166,   79,   24,  116,  157,  170,  173,  194,
 /*   190 */    80,  115,  114,  123,  122,  121,  109,   81,   94,  105,
 /*   200 */    95,  159,  190,  176,   70,   32,  172,  169,  165,   47,
 /*   210 */    46,   45,   44,   43,   42,   41,   40,   39,   38,   37,
 /*   220 */    36,   35,   62,   61,   71,   22,  200,   65,   64,   63,
 /*   230 */    60,   59,   58,   57,   56,   55,  164,  163,  162,  161,
 /*   240 */   160,    2,  151,  110,  146,  170,  173,  194,   80,  115,
 /*   250 */   114,  123,  122,  121,  109,   81,   94,  105,   95,  159,
 /*   260 */   190,  176,   70,  104,    8,  169,  165,  107,  323,  170,
 /*   270 */   173,  194,   80,  115,  114,  123,  122,  121,  109,   81,
 /*   280 */    94,  105,   95,  159,  190,  176,   70,  202,  199,  169,
 /*   290 */   165,  170,  143,  194,   80,  115,  114,  123,  122,  121,
 /*   300 */   109,   81,   94,  105,   95,  159,  190,  176,   70,  198,
 /*   310 */     5,  169,  165,  166,   79,  197,  130,  148,  103,  170,
 /*   320 */   195,  194,   80,  115,  114,  123,  122,  121,  109,   81,
 /*   330 */    94,  105,   95,  159,  190,  176,   70,  203,  323,  169,
 /*   340 */   165,  204,   77,   76,   73,  201,   68,  323,   75,  128,
 /*   350 */     7,  145,   67,   62,   61,  148,  323,   21,   65,   64,
 /*   360 */    63,   60,   59,   58,   57,   56,   55,  164,  163,  162,
 /*   370 */   161,  160,    2,    5,  323,  323,  166,   79,  170,  193,
 /*   380 */   194,   80,  115,  114,  123,  122,  121,  109,   81,   94,
 /*   390 */   105,   95,  159,  190,  176,   70,  323,  323,  169,  165,
 /*   400 */    77,   76,   73,  201,   68,  129,   75,  128,    7,  323,
 /*   410 */    67,  149,  323,  323,  323,  323,   62,   61,  323,  323,
 /*   420 */   323,   65,   64,   63,   60,   59,   58,   57,   56,   55,
 /*   430 */   164,  163,  162,  161,  160,    2,  170,  127,  194,   80,
 /*   440 */   115,  114,  123,  122,  121,  109,   81,   94,  105,   95,
 /*   450 */   159,  190,  176,   70,  323,  323,  169,  165,  170,  192,
 /*   460 */   194,   80,  115,  114,  123,  122,  121,  109,   81,   94,
 /*   470 */   105,   95,  159,  190,  176,   70,  323,  323,  169,  165,
 /*   480 */   170,  153,  194,   80,  115,  114,  123,  122,  121,  109,
 /*   490 */    81,   94,  105,   95,  159,  190,  176,   70,  323,  323,
 /*   500 */   169,  165,  170,  144,  194,   80,  115,  114,  123,  122,
 /*   510 */   121,  109,   81,   94,  105,   95,  159,  190,  176,   70,
 /*   520 */   323,  323,  169,  165,  170,  142,  194,   80,  115,  114,
 /*   530 */   123,  122,  121,  109,   81,   94,  105,   95,  159,  190,
 /*   540 */   176,   70,  323,  323,  169,  165,  170,  141,  194,   80,
 /*   550 */   115,  114,  123,  122,  121,  109,   81,   94,  105,   95,
 /*   560 */   159,  190,  176,   70,  323,  323,  169,  165,  170,  140,
 /*   570 */   194,   80,  115,  114,  123,  122,  121,  109,   81,   94,
 /*   580 */   105,   95,  159,  190,  176,   70,  323,  323,  169,  165,
 /*   590 */   170,  139,  194,   80,  115,  114,  123,  122,  121,  109,
 /*   600 */    81,   94,  105,   95,  159,  190,  176,   70,  323,  323,
 /*   610 */   169,  165,  170,  138,  194,   80,  115,  114,  123,  122,
 /*   620 */   121,  109,   81,   94,  105,   95,  159,  190,  176,   70,
 /*   630 */   323,  323,  169,  165,  170,  137,  194,   80,  115,  114,
 /*   640 */   123,  122,  121,  109,   81,   94,  105,   95,  159,  190,
 /*   650 */   176,   70,  323,  323,  169,  165,  170,  136,  194,   80,
 /*   660 */   115,  114,  123,  122,  121,  109,   81,   94,  105,   95,
 /*   670 */   159,  190,  176,   70,  323,  323,  169,  165,  170,  135,
 /*   680 */   194,   80,  115,  114,  123,  122,  121,  109,   81,   94,
 /*   690 */   105,   95,  159,  190,  176,   70,  323,  323,  169,  165,
 /*   700 */   170,  134,  194,   80,  115,  114,  123,  122,  121,  109,
 /*   710 */    81,   94,  105,   95,  159,  190,  176,   70,  323,  323,
 /*   720 */   169,  165,  170,  133,  194,   80,  115,  114,  123,  122,
 /*   730 */   121,  109,   81,   94,  105,   95,  159,  190,  176,   70,
 /*   740 */   323,  323,  169,  165,  170,  132,  194,   80,  115,  114,
 /*   750 */   123,  122,  121,  109,   81,   94,  105,   95,  159,  190,
 /*   760 */   176,   70,  323,  323,  169,  165,  170,  156,  194,   80,
 /*   770 */   115,  114,  123,  122,  121,  109,   81,   94,  105,   95,
 /*   780 */   159,  190,  176,   70,  323,  323,  169,  165,  170,  150,
 /*   790 */   194,   80,  115,  114,  123,  122,  121,  109,   81,   94,
 /*   800 */   105,   95,  159,  190,  176,   70,  323,  170,  169,  165,
 /*   810 */   111,  323,  102,  123,  122,  121,  109,   81,   94,  105,
 /*   820 */    95,  159,  190,  176,   70,  323,  170,  169,  165,  111,
 /*   830 */   323,  323,  126,  122,  121,  109,   81,   94,  105,   95,
 /*   840 */   159,  190,  176,   70,  323,  170,  169,  165,  111,  323,
 /*   850 */   323,  118,  122,  121,  109,   81,   94,  105,   95,  159,
 /*   860 */   190,  176,   70,  323,  170,  169,  165,  111,  323,  323,
 /*   870 */   323,  125,  121,  109,   81,   94,  105,   95,  159,  190,
 /*   880 */   176,   70,  323,  323,  169,  165,   25,   20,   19,   18,
 /*   890 */    17,   16,   15,   14,   13,   12,   11,   10,  170,  323,
 /*   900 */   323,  111,  323,  323,  323,  323,  124,  109,   81,   94,
 /*   910 */   105,   95,  159,  190,  176,   70,  323,  323,  169,  165,
 /*   920 */   323,  170,  323,  323,  111,  323,  323,  178,  177,  323,
 /*   930 */   113,   81,   94,  105,   95,  159,  190,  176,   70,  323,
 /*   940 */   170,  169,  165,  111,  323,  323,  323,  323,  323,  323,
 /*   950 */    83,   94,  105,   95,  159,  190,  176,   70,  205,  323,
 /*   960 */   169,  165,  170,  323,  323,  111,  323,  323,  323,  323,
 /*   970 */    26,  323,   82,   94,  105,   95,  159,  190,  176,   70,
 /*   980 */   323,  323,  169,  165,  170,  323,  323,  111,  323,  323,
 /*   990 */   323,  323,  323,  323,  323,  100,  105,   95,  159,  190,
 /*  1000 */   176,   70,  323,  170,  169,  165,  111,  323,  323,  323,
 /*  1010 */   323,  323,  323,  323,   98,  105,   95,  159,  190,  176,
 /*  1020 */    70,  323,  323,  169,  165,  170,  323,  323,  111,  323,
 /*  1030 */   323,  323,  323,  323,  323,  323,   96,  105,   95,  159,
 /*  1040 */   190,  176,   70,  323,  323,  169,  165,  170,  323,  323,
 /*  1050 */   111,  323,  323,  323,  323,  323,  323,  323,   93,  105,
 /*  1060 */    95,  159,  190,  176,   70,  323,  323,  169,  165,  170,
 /*  1070 */   323,  323,  111,  323,  323,  323,  323,  323,  323,  323,
 /*  1080 */    92,  105,   95,  159,  190,  176,   70,  323,  323,  169,
 /*  1090 */   165,  170,  323,  323,  111,  323,  323,  323,  323,  323,
 /*  1100 */   323,  323,   91,  105,   95,  159,  190,  176,   70,  323,
 /*  1110 */   323,  169,  165,  170,  323,  323,  111,  323,  323,  323,
 /*  1120 */   323,  323,  323,  323,   90,  105,   95,  159,  190,  176,
 /*  1130 */    70,  323,  323,  169,  165,  170,  323,  323,  111,  323,
 /*  1140 */   323,  323,  323,  323,  323,  323,   89,  105,   95,  159,
 /*  1150 */   190,  176,   70,  323,  323,  169,  165,  170,  323,  323,
 /*  1160 */   111,  323,  323,  323,  323,  323,  323,  323,   88,  105,
 /*  1170 */    95,  159,  190,  176,   70,  323,  323,  169,  165,  170,
 /*  1180 */   323,  323,  111,  323,  323,  323,  323,  323,  323,  323,
 /*  1190 */    87,  105,   95,  159,  190,  176,   70,  323,  323,  169,
 /*  1200 */   165,  170,  323,  323,  111,  323,  323,  323,  323,  323,
 /*  1210 */   323,  323,   86,  105,   95,  159,  190,  176,   70,  323,
 /*  1220 */   323,  169,  165,  170,  323,  323,  111,  323,  323,  323,
 /*  1230 */   323,  323,  323,  323,   85,  105,   95,  159,  190,  176,
 /*  1240 */    70,  323,  323,  169,  165,  170,  323,  323,  111,  323,
 /*  1250 */   323,  323,  323,  323,  323,  323,   84,  105,   95,  159,
 /*  1260 */   190,  176,   70,  323,  323,  169,  165,  170,  323,  323,
 /*  1270 */   111,  323,  323,  323,  323,  323,  323,  323,  323,  112,
 /*  1280 */    95,  159,  190,  176,   70,  323,  323,  169,  165,  170,
 /*  1290 */     5,  323,  111,  166,   79,  323,  323,  323,  323,  323,
 /*  1300 */   323,  108,   95,  159,  190,  176,   70,  323,  323,  169,
 /*  1310 */   165,  170,  323,  323,  111,  323,  323,  323,  323,  170,
 /*  1320 */   323,  323,  111,  106,   95,  159,  190,  176,   70,  323,
 /*  1330 */   323,  169,  165,  191,  190,  176,   70,  323,  323,  169,
 /*  1340 */   165,  170,  323,  323,  111,  323,  323,  164,  163,  162,
 /*  1350 */   161,  160,    2,  323,   99,  159,  190,  176,   70,  323,
 /*  1360 */   323,  169,  165,  170,  323,  323,  111,  323,  323,  323,
 /*  1370 */   323,  323,  323,  323,  323,  323,   97,  159,  190,  176,
 /*  1380 */    70,  323,  170,  169,  165,  111,  323,  323,  323,  323,
 /*  1390 */   170,  323,  323,  111,  323,  323,  189,  190,  176,   70,
 /*  1400 */   323,  323,  169,  165,  188,  190,  176,   70,  323,  170,
 /*  1410 */   169,  165,  111,  323,  323,  323,  323,  170,  323,  323,
 /*  1420 */   111,  323,  323,  187,  190,  176,   70,  323,  323,  169,
 /*  1430 */   165,  186,  190,  176,   70,  323,  323,  169,  165,  170,
 /*  1440 */   323,  323,  111,  323,  323,  323,  323,  323,  323,  323,
 /*  1450 */   323,  323,  323,  185,  190,  176,   70,  323,  170,  169,
 /*  1460 */   165,  111,  323,  323,  323,  323,  170,  323,  323,  111,
 /*  1470 */   323,  323,  184,  190,  176,   70,  323,  323,  169,  165,
 /*  1480 */   183,  190,  176,   70,  323,  170,  169,  165,  111,  323,
 /*  1490 */   323,  323,  323,  170,  323,  323,  111,  323,  323,  182,
 /*  1500 */   190,  176,   70,  323,  323,  169,  165,  181,  190,  176,
 /*  1510 */    70,  323,  323,  169,  165,  170,  323,  323,  111,  323,
 /*  1520 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  180,
 /*  1530 */   190,  176,   70,  323,  170,  169,  165,  111,  323,  323,
 /*  1540 */   323,  323,  170,  323,  323,  111,  323,  323,  179,  190,
 /*  1550 */   176,   70,  323,  323,  169,  165,  171,  190,  176,   70,
 /*  1560 */   323,  170,  169,  165,  111,  323,  323,  323,  323,  323,
 /*  1570 */   323,  323,  323,  323,  323,  167,  190,  176,   70,  323,
 /*  1580 */   323,  169,  165,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*    10 */    81,   82,   83,   84,   85,   86,   87,   88,   89,   90,
 /*    20 */    91,   92,    1,    2,   95,   96,   72,   73,   74,   75,
 /*    30 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   85,
 /*    40 */    86,   87,   88,   89,   90,   91,   92,   30,   31,   95,
 /*    50 */    96,    4,    5,    7,    7,    8,    9,   73,   11,   75,
 /*    60 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   85,
 /*    70 */    86,   87,   88,   89,   90,   91,   92,   48,   49,   95,
 /*    80 */    96,    4,    5,   99,    7,    8,    9,    5,   11,   45,
 /*    90 */    46,   47,   50,   51,   52,   48,   49,   10,    8,   12,
 /*   100 */    53,   54,   55,   56,   57,   58,   59,   60,   61,   62,
 /*   110 */    63,   64,   65,   66,   67,    5,   54,   55,    8,    9,
 /*   120 */    26,   10,   12,   12,   27,   75,   76,   77,   78,   79,
 /*   130 */    80,   81,   82,   83,   84,   85,   86,   87,   88,   89,
 /*   140 */    90,   91,   92,  100,  101,   95,   96,   97,   98,   67,
 /*   150 */    12,   69,   62,   28,   64,    6,   93,   94,   48,   49,
 /*   160 */    75,   12,    3,   53,   54,   55,   56,   57,   58,   59,
 /*   170 */    60,   61,   62,   63,   64,   65,   66,   67,   68,    5,
 /*   180 */    95,   96,    8,    9,   25,   73,   12,   75,   76,   77,
 /*   190 */    78,   79,   80,   81,   82,   83,   84,   85,   86,   87,
 /*   200 */    88,   89,   90,   91,   92,   29,   68,   95,   96,   32,
 /*   210 */    33,   34,   35,   36,   37,   38,   39,   40,   41,   42,
 /*   220 */    43,   44,   48,   49,   72,   26,   74,   53,   54,   55,
 /*   230 */    56,   57,   58,   59,   60,   61,   62,   63,   64,   65,
 /*   240 */    66,   67,   68,   73,    8,   75,   76,   77,   78,   79,
 /*   250 */    80,   81,   82,   83,   84,   85,   86,   87,   88,   89,
 /*   260 */    90,   91,   92,   99,   97,   95,   96,   73,  103,   75,
 /*   270 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   85,
 /*   280 */    86,   87,   88,   89,   90,   91,   92,   74,   74,   95,
 /*   290 */    96,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   300 */    84,   85,   86,   87,   88,   89,   90,   91,   92,   74,
 /*   310 */     5,   95,   96,    8,    9,   74,   74,   12,  102,   75,
 /*   320 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   85,
 /*   330 */    86,   87,   88,   89,   90,   91,   92,   74,  103,   95,
 /*   340 */    96,    0,    1,    2,    3,    4,    5,  103,    7,    8,
 /*   350 */     9,    6,   11,   48,   49,   12,  103,   12,   53,   54,
 /*   360 */    55,   56,   57,   58,   59,   60,   61,   62,   63,   64,
 /*   370 */    65,   66,   67,    5,  103,  103,    8,    9,   75,   76,
 /*   380 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   390 */    87,   88,   89,   90,   91,   92,  103,  103,   95,   96,
 /*   400 */     1,    2,    3,    4,    5,    6,    7,    8,    9,  103,
 /*   410 */    11,   68,  103,  103,  103,  103,   48,   49,  103,  103,
 /*   420 */   103,   53,   54,   55,   56,   57,   58,   59,   60,   61,
 /*   430 */    62,   63,   64,   65,   66,   67,   75,   76,   77,   78,
 /*   440 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   88,
 /*   450 */    89,   90,   91,   92,  103,  103,   95,   96,   75,   76,
 /*   460 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   470 */    87,   88,   89,   90,   91,   92,  103,  103,   95,   96,
 /*   480 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   490 */    85,   86,   87,   88,   89,   90,   91,   92,  103,  103,
 /*   500 */    95,   96,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   510 */    83,   84,   85,   86,   87,   88,   89,   90,   91,   92,
 /*   520 */   103,  103,   95,   96,   75,   76,   77,   78,   79,   80,
 /*   530 */    81,   82,   83,   84,   85,   86,   87,   88,   89,   90,
 /*   540 */    91,   92,  103,  103,   95,   96,   75,   76,   77,   78,
 /*   550 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   88,
 /*   560 */    89,   90,   91,   92,  103,  103,   95,   96,   75,   76,
 /*   570 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   580 */    87,   88,   89,   90,   91,   92,  103,  103,   95,   96,
 /*   590 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   600 */    85,   86,   87,   88,   89,   90,   91,   92,  103,  103,
 /*   610 */    95,   96,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   620 */    83,   84,   85,   86,   87,   88,   89,   90,   91,   92,
 /*   630 */   103,  103,   95,   96,   75,   76,   77,   78,   79,   80,
 /*   640 */    81,   82,   83,   84,   85,   86,   87,   88,   89,   90,
 /*   650 */    91,   92,  103,  103,   95,   96,   75,   76,   77,   78,
 /*   660 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   88,
 /*   670 */    89,   90,   91,   92,  103,  103,   95,   96,   75,   76,
 /*   680 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   690 */    87,   88,   89,   90,   91,   92,  103,  103,   95,   96,
 /*   700 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   710 */    85,   86,   87,   88,   89,   90,   91,   92,  103,  103,
 /*   720 */    95,   96,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   730 */    83,   84,   85,   86,   87,   88,   89,   90,   91,   92,
 /*   740 */   103,  103,   95,   96,   75,   76,   77,   78,   79,   80,
 /*   750 */    81,   82,   83,   84,   85,   86,   87,   88,   89,   90,
 /*   760 */    91,   92,  103,  103,   95,   96,   75,   76,   77,   78,
 /*   770 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   88,
 /*   780 */    89,   90,   91,   92,  103,  103,   95,   96,   75,   76,
 /*   790 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   800 */    87,   88,   89,   90,   91,   92,  103,   75,   95,   96,
 /*   810 */    78,  103,   80,   81,   82,   83,   84,   85,   86,   87,
 /*   820 */    88,   89,   90,   91,   92,  103,   75,   95,   96,   78,
 /*   830 */   103,  103,   81,   82,   83,   84,   85,   86,   87,   88,
 /*   840 */    89,   90,   91,   92,  103,   75,   95,   96,   78,  103,
 /*   850 */   103,   81,   82,   83,   84,   85,   86,   87,   88,   89,
 /*   860 */    90,   91,   92,  103,   75,   95,   96,   78,  103,  103,
 /*   870 */   103,   82,   83,   84,   85,   86,   87,   88,   89,   90,
 /*   880 */    91,   92,  103,  103,   95,   96,   13,   14,   15,   16,
 /*   890 */    17,   18,   19,   20,   21,   22,   23,   24,   75,  103,
 /*   900 */   103,   78,  103,  103,  103,  103,   83,   84,   85,   86,
 /*   910 */    87,   88,   89,   90,   91,   92,  103,  103,   95,   96,
 /*   920 */   103,   75,  103,  103,   78,  103,  103,   54,   55,  103,
 /*   930 */    84,   85,   86,   87,   88,   89,   90,   91,   92,  103,
 /*   940 */    75,   95,   96,   78,  103,  103,  103,  103,  103,  103,
 /*   950 */    85,   86,   87,   88,   89,   90,   91,   92,    0,  103,
 /*   960 */    95,   96,   75,  103,  103,   78,  103,  103,  103,  103,
 /*   970 */    12,  103,   85,   86,   87,   88,   89,   90,   91,   92,
 /*   980 */   103,  103,   95,   96,   75,  103,  103,   78,  103,  103,
 /*   990 */   103,  103,  103,  103,  103,   86,   87,   88,   89,   90,
 /*  1000 */    91,   92,  103,   75,   95,   96,   78,  103,  103,  103,
 /*  1010 */   103,  103,  103,  103,   86,   87,   88,   89,   90,   91,
 /*  1020 */    92,  103,  103,   95,   96,   75,  103,  103,   78,  103,
 /*  1030 */   103,  103,  103,  103,  103,  103,   86,   87,   88,   89,
 /*  1040 */    90,   91,   92,  103,  103,   95,   96,   75,  103,  103,
 /*  1050 */    78,  103,  103,  103,  103,  103,  103,  103,   86,   87,
 /*  1060 */    88,   89,   90,   91,   92,  103,  103,   95,   96,   75,
 /*  1070 */   103,  103,   78,  103,  103,  103,  103,  103,  103,  103,
 /*  1080 */    86,   87,   88,   89,   90,   91,   92,  103,  103,   95,
 /*  1090 */    96,   75,  103,  103,   78,  103,  103,  103,  103,  103,
 /*  1100 */   103,  103,   86,   87,   88,   89,   90,   91,   92,  103,
 /*  1110 */   103,   95,   96,   75,  103,  103,   78,  103,  103,  103,
 /*  1120 */   103,  103,  103,  103,   86,   87,   88,   89,   90,   91,
 /*  1130 */    92,  103,  103,   95,   96,   75,  103,  103,   78,  103,
 /*  1140 */   103,  103,  103,  103,  103,  103,   86,   87,   88,   89,
 /*  1150 */    90,   91,   92,  103,  103,   95,   96,   75,  103,  103,
 /*  1160 */    78,  103,  103,  103,  103,  103,  103,  103,   86,   87,
 /*  1170 */    88,   89,   90,   91,   92,  103,  103,   95,   96,   75,
 /*  1180 */   103,  103,   78,  103,  103,  103,  103,  103,  103,  103,
 /*  1190 */    86,   87,   88,   89,   90,   91,   92,  103,  103,   95,
 /*  1200 */    96,   75,  103,  103,   78,  103,  103,  103,  103,  103,
 /*  1210 */   103,  103,   86,   87,   88,   89,   90,   91,   92,  103,
 /*  1220 */   103,   95,   96,   75,  103,  103,   78,  103,  103,  103,
 /*  1230 */   103,  103,  103,  103,   86,   87,   88,   89,   90,   91,
 /*  1240 */    92,  103,  103,   95,   96,   75,  103,  103,   78,  103,
 /*  1250 */   103,  103,  103,  103,  103,  103,   86,   87,   88,   89,
 /*  1260 */    90,   91,   92,  103,  103,   95,   96,   75,  103,  103,
 /*  1270 */    78,  103,  103,  103,  103,  103,  103,  103,  103,   87,
 /*  1280 */    88,   89,   90,   91,   92,  103,  103,   95,   96,   75,
 /*  1290 */     5,  103,   78,    8,    9,  103,  103,  103,  103,  103,
 /*  1300 */   103,   87,   88,   89,   90,   91,   92,  103,  103,   95,
 /*  1310 */    96,   75,  103,  103,   78,  103,  103,  103,  103,   75,
 /*  1320 */   103,  103,   78,   87,   88,   89,   90,   91,   92,  103,
 /*  1330 */   103,   95,   96,   89,   90,   91,   92,  103,  103,   95,
 /*  1340 */    96,   75,  103,  103,   78,  103,  103,   62,   63,   64,
 /*  1350 */    65,   66,   67,  103,   88,   89,   90,   91,   92,  103,
 /*  1360 */   103,   95,   96,   75,  103,  103,   78,  103,  103,  103,
 /*  1370 */   103,  103,  103,  103,  103,  103,   88,   89,   90,   91,
 /*  1380 */    92,  103,   75,   95,   96,   78,  103,  103,  103,  103,
 /*  1390 */    75,  103,  103,   78,  103,  103,   89,   90,   91,   92,
 /*  1400 */   103,  103,   95,   96,   89,   90,   91,   92,  103,   75,
 /*  1410 */    95,   96,   78,  103,  103,  103,  103,   75,  103,  103,
 /*  1420 */    78,  103,  103,   89,   90,   91,   92,  103,  103,   95,
 /*  1430 */    96,   89,   90,   91,   92,  103,  103,   95,   96,   75,
 /*  1440 */   103,  103,   78,  103,  103,  103,  103,  103,  103,  103,
 /*  1450 */   103,  103,  103,   89,   90,   91,   92,  103,   75,   95,
 /*  1460 */    96,   78,  103,  103,  103,  103,   75,  103,  103,   78,
 /*  1470 */   103,  103,   89,   90,   91,   92,  103,  103,   95,   96,
 /*  1480 */    89,   90,   91,   92,  103,   75,   95,   96,   78,  103,
 /*  1490 */   103,  103,  103,   75,  103,  103,   78,  103,  103,   89,
 /*  1500 */    90,   91,   92,  103,  103,   95,   96,   89,   90,   91,
 /*  1510 */    92,  103,  103,   95,   96,   75,  103,  103,   78,  103,
 /*  1520 */   103,  103,  103,  103,  103,  103,  103,  103,  103,   89,
 /*  1530 */    90,   91,   92,  103,   75,   95,   96,   78,  103,  103,
 /*  1540 */   103,  103,   75,  103,  103,   78,  103,  103,   89,   90,
 /*  1550 */    91,   92,  103,  103,   95,   96,   89,   90,   91,   92,
 /*  1560 */   103,   75,   95,   96,   78,  103,  103,  103,  103,  103,
 /*  1570 */   103,  103,  103,  103,  103,   89,   90,   91,   92,  103,
 /*  1580 */   103,   95,   96,
};
#define YY_SHIFT_USE_DFLT (-1)
#define YY_SHIFT_COUNT (128)
#define YY_SHIFT_MIN   (0)
#define YY_SHIFT_MAX   (1285)
static const short yy_shift_ofst[] = {
 /*     0 */    47,   47,  305,  368,  368,  368,  368,  368,  174,  110,
 /*    10 */   368,  368,  368,  368,  368,  368,  368,  368,  368,  368,
 /*    20 */   368,  368,  368,  368,  368,  368,  368,  368,  368,  368,
 /*    30 */   368,  368,  368,  368,  368,  368,  368,  368,  368,  368,
 /*    40 */   368,  368,  368,  368,  368,  368,  368,  368,  368,  368,
 /*    50 */   368,  368,  368,  368,  368,  368,  368,  368,  368,  368,
 /*    60 */   368,  368,  368,  368,  368,  368,  368, 1285,   77,   90,
 /*    70 */    82,  399,  341,   77,   77,   77,   77,   77,  343,   -1,
 /*    80 */   873,  177,  177,  177,   44,   44,   44,   44,   44,   44,
 /*    90 */    44,   44,   44,   44,   44,   42,   44,   42,   44,   42,
 /*   100 */    44,  958,   21,  345,  111,   29,   29,  149,   29,   17,
 /*   110 */   138,   62,   29,   17,   21,  159,   87,   46,   97,  236,
 /*   120 */   199,  176,  125,   97,  176,  125,   97,   94,   46,
};
#define YY_REDUCE_USE_DFLT (-72)
#define YY_REDUCE_COUNT (79)
#define YY_REDUCE_MIN   (-71)
#define YY_REDUCE_MAX   (1486)
static const short yy_reduce_ofst[] = {
 /*     0 */   -71,  -46,   50,  -16,  216,  194,  170,  112,  713,  691,
 /*    10 */   669,  647,  625,  603,  581,  559,  537,  515,  493,  471,
 /*    20 */   449,  427,  405,  383,  361,  303,  244,  732,  770,  751,
 /*    30 */   789,  823,  846,  887,  865, 1170, 1148, 1126, 1104, 1082,
 /*    40 */  1060, 1038, 1016,  994,  972,  950,  928,  909, 1236, 1214,
 /*    50 */  1192, 1288, 1266, 1486, 1467, 1459, 1440, 1418, 1410, 1391,
 /*    60 */  1383, 1364, 1342, 1334, 1315, 1307, 1244,   85,  152,   43,
 /*    70 */    63,  263,  263,  242,  241,  235,  214,  213,  167,  164,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   321,  321,  321,  311,  318,  321,  321,  321,  321,  321,
 /*    10 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*    20 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*    30 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*    40 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*    50 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*    60 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*    70 */   289,  321,  321,  321,  321,  321,  321,  321,  321,  311,
 /*    80 */   285,  245,  247,  246,  261,  260,  259,  258,  257,  256,
 /*    90 */   255,  254,  253,  252,  248,  266,  251,  268,  250,  267,
 /*   100 */   249,  321,  235,  321,  321,  262,  265,  321,  264,  243,
 /*   110 */   321,  285,  263,  244,  234,  232,  321,  295,  238,  321,
 /*   120 */   321,  241,  239,  236,  242,  240,  237,  321,  321,  212,
 /*   130 */   210,  216,  231,  230,  229,  228,  227,  226,  225,  224,
 /*   140 */   223,  222,  221,  319,  320,  317,  316,  307,  305,  304,
 /*   150 */   309,  303,  314,  313,  312,  310,  308,  306,  302,  269,
 /*   160 */   301,  300,  299,  298,  297,  296,  295,  272,  294,  293,
 /*   170 */   291,  271,  315,  217,  292,  290,  288,  287,  286,  284,
 /*   180 */   283,  282,  281,  280,  279,  278,  277,  276,  275,  274,
 /*   190 */   273,  270,  233,  220,  219,  218,  215,  214,  213,  209,
 /*   200 */   206,  211,  208,  207,
};

/* The next table maps tokens into fallback tokens.  If a construct
** like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
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
  YYACTIONTYPE stateno;  /* The state-number */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyidxMax;                 /* Maximum value of yyidx */
#endif
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
  "SHIFTR_ASSIGN",  "SHIFTRR_ASSIGN",  "AND_ASSIGN",    "XOR_ASSIGN",  
  "OR_ASSIGN",     "QUESTION",      "COLON",         "BITWISE_OR",  
  "BITWISE_XOR",   "BITWISE_AND",   "EQUAL",         "NOT_EQUAL",   
  "LESS",          "GREATER",       "LESS_EQUAL",    "GREATER_EQUAL",
  "IN",            "MATCH",         "NEAR",          "NEAR2",       
  "SIMILAR",       "TERM_EXTRACT",  "LCP",           "PREFIX",      
  "SUFFIX",        "SHIFTL",        "SHIFTR",        "SHIFTRR",     
  "PLUS",          "MINUS",         "STAR",          "SLASH",       
  "MOD",           "DELETE",        "INCR",          "DECR",        
  "NOT",           "BITWISE_NOT",   "ADJUST",        "EXACT",       
  "PARTIAL",       "UNSPLIT",       "DECIMAL",       "HEX_INTEGER", 
  "STRING",        "BOOLEAN",       "NULL",          "BRACKETL",    
  "BRACKETR",      "DOT",           "error",         "input",       
  "query",         "expression",    "query_element",  "primary_expression",
  "assignment_expression",  "conditional_expression",  "lefthand_side_expression",  "logical_or_expression",
  "logical_and_expression",  "bitwise_or_expression",  "bitwise_xor_expression",  "bitwise_and_expression",
  "equality_expression",  "relational_expression",  "shift_expression",  "additive_expression",
  "multiplicative_expression",  "unary_expression",  "postfix_expression",  "call_expression",
  "member_expression",  "arguments",     "member_expression_part",  "object_literal",
  "array_literal",  "elision",       "element_list",  "property_name_and_value_list",
  "property_name_and_value",  "property_name",  "argument_list",
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
 /*  23 */ "assignment_expression ::= lefthand_side_expression SHIFTR_ASSIGN assignment_expression",
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
 /*  52 */ "relational_expression ::= relational_expression NEAR2 shift_expression",
 /*  53 */ "relational_expression ::= relational_expression SIMILAR shift_expression",
 /*  54 */ "relational_expression ::= relational_expression TERM_EXTRACT shift_expression",
 /*  55 */ "relational_expression ::= relational_expression LCP shift_expression",
 /*  56 */ "relational_expression ::= relational_expression PREFIX shift_expression",
 /*  57 */ "relational_expression ::= relational_expression SUFFIX shift_expression",
 /*  58 */ "shift_expression ::= additive_expression",
 /*  59 */ "shift_expression ::= shift_expression SHIFTL additive_expression",
 /*  60 */ "shift_expression ::= shift_expression SHIFTR additive_expression",
 /*  61 */ "shift_expression ::= shift_expression SHIFTRR additive_expression",
 /*  62 */ "additive_expression ::= multiplicative_expression",
 /*  63 */ "additive_expression ::= additive_expression PLUS multiplicative_expression",
 /*  64 */ "additive_expression ::= additive_expression MINUS multiplicative_expression",
 /*  65 */ "multiplicative_expression ::= unary_expression",
 /*  66 */ "multiplicative_expression ::= multiplicative_expression STAR unary_expression",
 /*  67 */ "multiplicative_expression ::= multiplicative_expression SLASH unary_expression",
 /*  68 */ "multiplicative_expression ::= multiplicative_expression MOD unary_expression",
 /*  69 */ "unary_expression ::= postfix_expression",
 /*  70 */ "unary_expression ::= DELETE unary_expression",
 /*  71 */ "unary_expression ::= INCR unary_expression",
 /*  72 */ "unary_expression ::= DECR unary_expression",
 /*  73 */ "unary_expression ::= PLUS unary_expression",
 /*  74 */ "unary_expression ::= MINUS unary_expression",
 /*  75 */ "unary_expression ::= NOT unary_expression",
 /*  76 */ "unary_expression ::= BITWISE_NOT unary_expression",
 /*  77 */ "unary_expression ::= ADJUST unary_expression",
 /*  78 */ "unary_expression ::= EXACT unary_expression",
 /*  79 */ "unary_expression ::= PARTIAL unary_expression",
 /*  80 */ "unary_expression ::= UNSPLIT unary_expression",
 /*  81 */ "postfix_expression ::= lefthand_side_expression",
 /*  82 */ "postfix_expression ::= lefthand_side_expression INCR",
 /*  83 */ "postfix_expression ::= lefthand_side_expression DECR",
 /*  84 */ "lefthand_side_expression ::= call_expression",
 /*  85 */ "lefthand_side_expression ::= member_expression",
 /*  86 */ "call_expression ::= member_expression arguments",
 /*  87 */ "member_expression ::= primary_expression",
 /*  88 */ "member_expression ::= member_expression member_expression_part",
 /*  89 */ "primary_expression ::= object_literal",
 /*  90 */ "primary_expression ::= PARENL expression PARENR",
 /*  91 */ "primary_expression ::= IDENTIFIER",
 /*  92 */ "primary_expression ::= array_literal",
 /*  93 */ "primary_expression ::= DECIMAL",
 /*  94 */ "primary_expression ::= HEX_INTEGER",
 /*  95 */ "primary_expression ::= STRING",
 /*  96 */ "primary_expression ::= BOOLEAN",
 /*  97 */ "primary_expression ::= NULL",
 /*  98 */ "array_literal ::= BRACKETL elision BRACKETR",
 /*  99 */ "array_literal ::= BRACKETL element_list elision BRACKETR",
 /* 100 */ "array_literal ::= BRACKETL element_list BRACKETR",
 /* 101 */ "elision ::= COMMA",
 /* 102 */ "elision ::= elision COMMA",
 /* 103 */ "element_list ::= assignment_expression",
 /* 104 */ "element_list ::= elision assignment_expression",
 /* 105 */ "element_list ::= element_list elision assignment_expression",
 /* 106 */ "object_literal ::= BRACEL property_name_and_value_list BRACER",
 /* 107 */ "property_name_and_value_list ::=",
 /* 108 */ "property_name_and_value_list ::= property_name_and_value_list COMMA property_name_and_value",
 /* 109 */ "property_name_and_value ::= property_name COLON assignment_expression",
 /* 110 */ "property_name ::= IDENTIFIER|STRING|DECIMAL",
 /* 111 */ "member_expression_part ::= BRACKETL expression BRACKETR",
 /* 112 */ "member_expression_part ::= DOT IDENTIFIER",
 /* 113 */ "arguments ::= PARENL argument_list PARENR",
 /* 114 */ "argument_list ::=",
 /* 115 */ "argument_list ::= assignment_expression",
 /* 116 */ "argument_list ::= argument_list COMMA assignment_expression",
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
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyidxMax = 0;
#endif
#if YYSTACKDEPTH<=0
    pParser->yystack = NULL;
    pParser->yystksz = 0;
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
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  grn_expr_parserARG_FETCH;
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
  yy_destructor(pParser, yymajor, &yytos->minor);
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
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int grn_expr_parserStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyidxMax;
}
#endif

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
 
  if( stateno>YY_SHIFT_COUNT
   || (i = yy_shift_ofst[stateno])==YY_SHIFT_USE_DFLT ){
    return yy_default[stateno];
  }
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    if( iLookAhead>0 ){
#ifdef YYFALLBACK
      YYCODETYPE iFallback;            /* Fallback token */
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
        if( 
#if YY_SHIFT_MIN+YYWILDCARD<0
          j>=0 &&
#endif
#if YY_SHIFT_MAX+YYWILDCARD>=YY_ACTTAB_COUNT
          j<YY_ACTTAB_COUNT &&
#endif
          yy_lookahead[j]==YYWILDCARD
        ){
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
  if( stateno>YY_REDUCE_COUNT ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_COUNT );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_ACTTAB_COUNT );
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
  YYMINORTYPE *yypMinor         /* Pointer to the minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( yypParser->yyidx>yypParser->yyidxMax ){
    yypParser->yyidxMax = yypParser->yyidx;
  }
#endif
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
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
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
  { 71, 1 },
  { 71, 1 },
  { 72, 1 },
  { 72, 2 },
  { 72, 3 },
  { 72, 3 },
  { 72, 3 },
  { 74, 1 },
  { 74, 3 },
  { 74, 2 },
  { 74, 3 },
  { 74, 3 },
  { 74, 2 },
  { 73, 1 },
  { 73, 3 },
  { 76, 1 },
  { 76, 3 },
  { 76, 3 },
  { 76, 3 },
  { 76, 3 },
  { 76, 3 },
  { 76, 3 },
  { 76, 3 },
  { 76, 3 },
  { 76, 3 },
  { 76, 3 },
  { 76, 3 },
  { 76, 3 },
  { 77, 1 },
  { 77, 5 },
  { 79, 1 },
  { 79, 3 },
  { 80, 1 },
  { 80, 3 },
  { 80, 3 },
  { 81, 1 },
  { 81, 3 },
  { 82, 1 },
  { 82, 3 },
  { 83, 1 },
  { 83, 3 },
  { 84, 1 },
  { 84, 3 },
  { 84, 3 },
  { 85, 1 },
  { 85, 3 },
  { 85, 3 },
  { 85, 3 },
  { 85, 3 },
  { 85, 3 },
  { 85, 3 },
  { 85, 3 },
  { 85, 3 },
  { 85, 3 },
  { 85, 3 },
  { 85, 3 },
  { 85, 3 },
  { 85, 3 },
  { 86, 1 },
  { 86, 3 },
  { 86, 3 },
  { 86, 3 },
  { 87, 1 },
  { 87, 3 },
  { 87, 3 },
  { 88, 1 },
  { 88, 3 },
  { 88, 3 },
  { 88, 3 },
  { 89, 1 },
  { 89, 2 },
  { 89, 2 },
  { 89, 2 },
  { 89, 2 },
  { 89, 2 },
  { 89, 2 },
  { 89, 2 },
  { 89, 2 },
  { 89, 2 },
  { 89, 2 },
  { 89, 2 },
  { 90, 1 },
  { 90, 2 },
  { 90, 2 },
  { 78, 1 },
  { 78, 1 },
  { 91, 2 },
  { 92, 1 },
  { 92, 2 },
  { 75, 1 },
  { 75, 3 },
  { 75, 1 },
  { 75, 1 },
  { 75, 1 },
  { 75, 1 },
  { 75, 1 },
  { 75, 1 },
  { 75, 1 },
  { 96, 3 },
  { 96, 4 },
  { 96, 3 },
  { 97, 1 },
  { 97, 2 },
  { 98, 1 },
  { 98, 2 },
  { 98, 3 },
  { 95, 3 },
  { 99, 0 },
  { 99, 3 },
  { 100, 3 },
  { 101, 1 },
  { 94, 3 },
  { 94, 2 },
  { 93, 3 },
  { 102, 0 },
  { 102, 1 },
  { 102, 3 },
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
      case 3: /* query ::= query query_element */
#line 29 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, grn_int32_value_at(&efsi->op_stack, -1), 2);
}
#line 1255 "expr.c"
        break;
      case 4: /* query ::= query LOGICAL_AND query_element */
      case 33: /* logical_and_expression ::= logical_and_expression LOGICAL_AND bitwise_or_expression */ yytestcase(yyruleno==33);
#line 32 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND, 2);
}
#line 1263 "expr.c"
        break;
      case 5: /* query ::= query LOGICAL_BUT query_element */
      case 34: /* logical_and_expression ::= logical_and_expression LOGICAL_BUT bitwise_or_expression */ yytestcase(yyruleno==34);
#line 35 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BUT, 2);
}
#line 1271 "expr.c"
        break;
      case 6: /* query ::= query LOGICAL_OR query_element */
      case 31: /* logical_or_expression ::= logical_or_expression LOGICAL_OR logical_and_expression */ yytestcase(yyruleno==31);
#line 38 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_OR, 2);
}
#line 1279 "expr.c"
        break;
      case 9: /* query_element ::= RELATIVE_OP query_element */
#line 45 "expr.y"
{
  int mode;
  GRN_UINT32_POP(&efsi->mode_stack, mode);
}
#line 1287 "expr.c"
        break;
      case 10: /* query_element ::= IDENTIFIER RELATIVE_OP query_element */
#line 49 "expr.y"
{
  int mode;
  grn_obj *c;
  GRN_PTR_POP(&efsi->column_stack, c);
  GRN_UINT32_POP(&efsi->mode_stack, mode);
}
#line 1297 "expr.c"
        break;
      case 11: /* query_element ::= BRACEL expression BRACER */
      case 12: /* query_element ::= EVAL primary_expression */ yytestcase(yyruleno==12);
#line 55 "expr.y"
{
  efsi->flags = efsi->default_flags;
}
#line 1305 "expr.c"
        break;
      case 14: /* expression ::= expression COMMA assignment_expression */
#line 63 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_COMMA, 2);
}
#line 1312 "expr.c"
        break;
      case 16: /* assignment_expression ::= lefthand_side_expression ASSIGN assignment_expression */
#line 68 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_ASSIGN, 2);
}
#line 1319 "expr.c"
        break;
      case 17: /* assignment_expression ::= lefthand_side_expression STAR_ASSIGN assignment_expression */
#line 71 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_STAR_ASSIGN, 2);
}
#line 1326 "expr.c"
        break;
      case 18: /* assignment_expression ::= lefthand_side_expression SLASH_ASSIGN assignment_expression */
#line 74 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SLASH_ASSIGN, 2);
}
#line 1333 "expr.c"
        break;
      case 19: /* assignment_expression ::= lefthand_side_expression MOD_ASSIGN assignment_expression */
#line 77 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MOD_ASSIGN, 2);
}
#line 1340 "expr.c"
        break;
      case 20: /* assignment_expression ::= lefthand_side_expression PLUS_ASSIGN assignment_expression */
#line 80 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PLUS_ASSIGN, 2);
}
#line 1347 "expr.c"
        break;
      case 21: /* assignment_expression ::= lefthand_side_expression MINUS_ASSIGN assignment_expression */
#line 83 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MINUS_ASSIGN, 2);
}
#line 1354 "expr.c"
        break;
      case 22: /* assignment_expression ::= lefthand_side_expression SHIFTL_ASSIGN assignment_expression */
#line 86 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTL_ASSIGN, 2);
}
#line 1361 "expr.c"
        break;
      case 23: /* assignment_expression ::= lefthand_side_expression SHIFTR_ASSIGN assignment_expression */
#line 89 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTR_ASSIGN, 2);
}
#line 1368 "expr.c"
        break;
      case 24: /* assignment_expression ::= lefthand_side_expression SHIFTRR_ASSIGN assignment_expression */
#line 92 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTRR_ASSIGN, 2);
}
#line 1375 "expr.c"
        break;
      case 25: /* assignment_expression ::= lefthand_side_expression AND_ASSIGN assignment_expression */
#line 95 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND_ASSIGN, 2);
}
#line 1382 "expr.c"
        break;
      case 26: /* assignment_expression ::= lefthand_side_expression XOR_ASSIGN assignment_expression */
#line 98 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_XOR_ASSIGN, 2);
}
#line 1389 "expr.c"
        break;
      case 27: /* assignment_expression ::= lefthand_side_expression OR_ASSIGN assignment_expression */
#line 101 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_OR_ASSIGN, 2);
}
#line 1396 "expr.c"
        break;
      case 29: /* conditional_expression ::= logical_or_expression QUESTION assignment_expression COLON assignment_expression */
#line 106 "expr.y"
{
  grn_expr *e = (grn_expr *)efsi->e;
  e->codes[yymsp[-3].minor.yy0].nargs = yymsp[-1].minor.yy0 - yymsp[-3].minor.yy0;
  e->codes[yymsp[-1].minor.yy0].nargs = e->codes_curr - yymsp[-1].minor.yy0 - 1;
}
#line 1405 "expr.c"
        break;
      case 36: /* bitwise_or_expression ::= bitwise_or_expression BITWISE_OR bitwise_xor_expression */
#line 126 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_OR, 2);
}
#line 1412 "expr.c"
        break;
      case 38: /* bitwise_xor_expression ::= bitwise_xor_expression BITWISE_XOR bitwise_and_expression */
#line 131 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_XOR, 2);
}
#line 1419 "expr.c"
        break;
      case 40: /* bitwise_and_expression ::= bitwise_and_expression BITWISE_AND equality_expression */
#line 136 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_AND, 2);
}
#line 1426 "expr.c"
        break;
      case 42: /* equality_expression ::= equality_expression EQUAL relational_expression */
#line 141 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_EQUAL, 2);
}
#line 1433 "expr.c"
        break;
      case 43: /* equality_expression ::= equality_expression NOT_EQUAL relational_expression */
#line 144 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NOT_EQUAL, 2);
}
#line 1440 "expr.c"
        break;
      case 45: /* relational_expression ::= relational_expression LESS shift_expression */
#line 149 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_LESS, 2);
}
#line 1447 "expr.c"
        break;
      case 46: /* relational_expression ::= relational_expression GREATER shift_expression */
#line 152 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_GREATER, 2);
}
#line 1454 "expr.c"
        break;
      case 47: /* relational_expression ::= relational_expression LESS_EQUAL shift_expression */
#line 155 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_LESS_EQUAL, 2);
}
#line 1461 "expr.c"
        break;
      case 48: /* relational_expression ::= relational_expression GREATER_EQUAL shift_expression */
#line 158 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_GREATER_EQUAL, 2);
}
#line 1468 "expr.c"
        break;
      case 49: /* relational_expression ::= relational_expression IN shift_expression */
#line 161 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_IN, 2);
}
#line 1475 "expr.c"
        break;
      case 50: /* relational_expression ::= relational_expression MATCH shift_expression */
#line 164 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MATCH, 2);
}
#line 1482 "expr.c"
        break;
      case 51: /* relational_expression ::= relational_expression NEAR shift_expression */
#line 167 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NEAR, 2);
}
#line 1489 "expr.c"
        break;
      case 52: /* relational_expression ::= relational_expression NEAR2 shift_expression */
#line 170 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NEAR2, 2);
}
#line 1496 "expr.c"
        break;
      case 53: /* relational_expression ::= relational_expression SIMILAR shift_expression */
#line 173 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SIMILAR, 2);
}
#line 1503 "expr.c"
        break;
      case 54: /* relational_expression ::= relational_expression TERM_EXTRACT shift_expression */
#line 176 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_TERM_EXTRACT, 2);
}
#line 1510 "expr.c"
        break;
      case 55: /* relational_expression ::= relational_expression LCP shift_expression */
#line 179 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_LCP, 2);
}
#line 1517 "expr.c"
        break;
      case 56: /* relational_expression ::= relational_expression PREFIX shift_expression */
#line 182 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PREFIX, 2);
}
#line 1524 "expr.c"
        break;
      case 57: /* relational_expression ::= relational_expression SUFFIX shift_expression */
#line 185 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SUFFIX, 2);
}
#line 1531 "expr.c"
        break;
      case 59: /* shift_expression ::= shift_expression SHIFTL additive_expression */
#line 190 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTL, 2);
}
#line 1538 "expr.c"
        break;
      case 60: /* shift_expression ::= shift_expression SHIFTR additive_expression */
#line 193 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTR, 2);
}
#line 1545 "expr.c"
        break;
      case 61: /* shift_expression ::= shift_expression SHIFTRR additive_expression */
#line 196 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTRR, 2);
}
#line 1552 "expr.c"
        break;
      case 63: /* additive_expression ::= additive_expression PLUS multiplicative_expression */
#line 201 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PLUS, 2);
}
#line 1559 "expr.c"
        break;
      case 64: /* additive_expression ::= additive_expression MINUS multiplicative_expression */
#line 204 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MINUS, 2);
}
#line 1566 "expr.c"
        break;
      case 66: /* multiplicative_expression ::= multiplicative_expression STAR unary_expression */
#line 209 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_STAR, 2);
}
#line 1573 "expr.c"
        break;
      case 67: /* multiplicative_expression ::= multiplicative_expression SLASH unary_expression */
#line 212 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SLASH, 2);
}
#line 1580 "expr.c"
        break;
      case 68: /* multiplicative_expression ::= multiplicative_expression MOD unary_expression */
#line 215 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MOD, 2);
}
#line 1587 "expr.c"
        break;
      case 70: /* unary_expression ::= DELETE unary_expression */
#line 220 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_DELETE, 1);
}
#line 1594 "expr.c"
        break;
      case 71: /* unary_expression ::= INCR unary_expression */
#line 223 "expr.y"
{
  grn_ctx *ctx = efsi->ctx;
  grn_expr *e = (grn_expr *)(efsi->e);
  grn_expr_dfi *dfi_;
  unsigned int const_p;

  DFI_POP(e, dfi_);
  const_p = CONSTP(dfi_->code->value);
  DFI_PUT(e, dfi_->type, dfi_->domain, dfi_->code);
  if (const_p) {
    ERR(GRN_SYNTAX_ERROR,
        "constant can't be incremented (%*s)",
        efsi->str_end - efsi->str, efsi->str);
  } else {
    grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_INCR, 1);
  }
}
#line 1615 "expr.c"
        break;
      case 72: /* unary_expression ::= DECR unary_expression */
#line 240 "expr.y"
{
  grn_ctx *ctx = efsi->ctx;
  grn_expr *e = (grn_expr *)(efsi->e);
  grn_expr_dfi *dfi_;
  unsigned int const_p;

  DFI_POP(e, dfi_);
  const_p = CONSTP(dfi_->code->value);
  DFI_PUT(e, dfi_->type, dfi_->domain, dfi_->code);
  if (const_p) {
    ERR(GRN_SYNTAX_ERROR,
        "constant can't be decremented (%*s)",
        efsi->str_end - efsi->str, efsi->str);
  } else {
    grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_DECR, 1);
  }
}
#line 1636 "expr.c"
        break;
      case 73: /* unary_expression ::= PLUS unary_expression */
#line 257 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PLUS, 1);
}
#line 1643 "expr.c"
        break;
      case 74: /* unary_expression ::= MINUS unary_expression */
#line 260 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MINUS, 1);
}
#line 1650 "expr.c"
        break;
      case 75: /* unary_expression ::= NOT unary_expression */
#line 263 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NOT, 1);
}
#line 1657 "expr.c"
        break;
      case 76: /* unary_expression ::= BITWISE_NOT unary_expression */
#line 266 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_NOT, 1);
}
#line 1664 "expr.c"
        break;
      case 77: /* unary_expression ::= ADJUST unary_expression */
#line 269 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_ADJUST, 1);
}
#line 1671 "expr.c"
        break;
      case 78: /* unary_expression ::= EXACT unary_expression */
#line 272 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_EXACT, 1);
}
#line 1678 "expr.c"
        break;
      case 79: /* unary_expression ::= PARTIAL unary_expression */
#line 275 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PARTIAL, 1);
}
#line 1685 "expr.c"
        break;
      case 80: /* unary_expression ::= UNSPLIT unary_expression */
#line 278 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_UNSPLIT, 1);
}
#line 1692 "expr.c"
        break;
      case 82: /* postfix_expression ::= lefthand_side_expression INCR */
#line 283 "expr.y"
{
  grn_ctx *ctx = efsi->ctx;
  grn_expr *e = (grn_expr *)(efsi->e);
  grn_expr_dfi *dfi_;
  unsigned int const_p;

  DFI_POP(e, dfi_);
  const_p = CONSTP(dfi_->code->value);
  DFI_PUT(e, dfi_->type, dfi_->domain, dfi_->code);
  if (const_p) {
    ERR(GRN_SYNTAX_ERROR,
        "constant can't be incremented (%*s)",
        efsi->str_end - efsi->str, efsi->str);
  } else {
    grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_INCR_POST, 1);
  }
}
#line 1713 "expr.c"
        break;
      case 83: /* postfix_expression ::= lefthand_side_expression DECR */
#line 300 "expr.y"
{
  grn_ctx *ctx = efsi->ctx;
  grn_expr *e = (grn_expr *)(efsi->e);
  grn_expr_dfi *dfi_;
  unsigned int const_p;

  DFI_POP(e, dfi_);
  const_p = CONSTP(dfi_->code->value);
  DFI_PUT(e, dfi_->type, dfi_->domain, dfi_->code);
  if (const_p) {
    ERR(GRN_SYNTAX_ERROR,
        "constant can't be decremented (%*s)",
        efsi->str_end - efsi->str, efsi->str);
  } else {
    grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_DECR_POST, 1);
  }
}
#line 1734 "expr.c"
        break;
      case 86: /* call_expression ::= member_expression arguments */
#line 321 "expr.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_CALL, yymsp[0].minor.yy0);
}
#line 1741 "expr.c"
        break;
      case 113: /* arguments ::= PARENL argument_list PARENR */
#line 360 "expr.y"
{ yygotominor.yy0 = yymsp[-1].minor.yy0; }
#line 1746 "expr.c"
        break;
      case 114: /* argument_list ::= */
#line 361 "expr.y"
{ yygotominor.yy0 = 0; }
#line 1751 "expr.c"
        break;
      case 115: /* argument_list ::= assignment_expression */
#line 362 "expr.y"
{ yygotominor.yy0 = 1; }
#line 1756 "expr.c"
        break;
      case 116: /* argument_list ::= argument_list COMMA assignment_expression */
#line 363 "expr.y"
{ yygotominor.yy0 = yymsp[-2].minor.yy0 + 1; }
#line 1761 "expr.c"
        break;
      default:
      /* (0) input ::= query */ yytestcase(yyruleno==0);
      /* (1) input ::= expression */ yytestcase(yyruleno==1);
      /* (2) query ::= query_element */ yytestcase(yyruleno==2);
      /* (7) query_element ::= QSTRING */ yytestcase(yyruleno==7);
      /* (8) query_element ::= PARENL query PARENR */ yytestcase(yyruleno==8);
      /* (13) expression ::= assignment_expression */ yytestcase(yyruleno==13);
      /* (15) assignment_expression ::= conditional_expression */ yytestcase(yyruleno==15);
      /* (28) conditional_expression ::= logical_or_expression */ yytestcase(yyruleno==28);
      /* (30) logical_or_expression ::= logical_and_expression */ yytestcase(yyruleno==30);
      /* (32) logical_and_expression ::= bitwise_or_expression */ yytestcase(yyruleno==32);
      /* (35) bitwise_or_expression ::= bitwise_xor_expression */ yytestcase(yyruleno==35);
      /* (37) bitwise_xor_expression ::= bitwise_and_expression */ yytestcase(yyruleno==37);
      /* (39) bitwise_and_expression ::= equality_expression */ yytestcase(yyruleno==39);
      /* (41) equality_expression ::= relational_expression */ yytestcase(yyruleno==41);
      /* (44) relational_expression ::= shift_expression */ yytestcase(yyruleno==44);
      /* (58) shift_expression ::= additive_expression */ yytestcase(yyruleno==58);
      /* (62) additive_expression ::= multiplicative_expression */ yytestcase(yyruleno==62);
      /* (65) multiplicative_expression ::= unary_expression */ yytestcase(yyruleno==65);
      /* (69) unary_expression ::= postfix_expression */ yytestcase(yyruleno==69);
      /* (81) postfix_expression ::= lefthand_side_expression */ yytestcase(yyruleno==81);
      /* (84) lefthand_side_expression ::= call_expression */ yytestcase(yyruleno==84);
      /* (85) lefthand_side_expression ::= member_expression */ yytestcase(yyruleno==85);
      /* (87) member_expression ::= primary_expression */ yytestcase(yyruleno==87);
      /* (88) member_expression ::= member_expression member_expression_part */ yytestcase(yyruleno==88);
      /* (89) primary_expression ::= object_literal */ yytestcase(yyruleno==89);
      /* (90) primary_expression ::= PARENL expression PARENR */ yytestcase(yyruleno==90);
      /* (91) primary_expression ::= IDENTIFIER */ yytestcase(yyruleno==91);
      /* (92) primary_expression ::= array_literal */ yytestcase(yyruleno==92);
      /* (93) primary_expression ::= DECIMAL */ yytestcase(yyruleno==93);
      /* (94) primary_expression ::= HEX_INTEGER */ yytestcase(yyruleno==94);
      /* (95) primary_expression ::= STRING */ yytestcase(yyruleno==95);
      /* (96) primary_expression ::= BOOLEAN */ yytestcase(yyruleno==96);
      /* (97) primary_expression ::= NULL */ yytestcase(yyruleno==97);
      /* (98) array_literal ::= BRACKETL elision BRACKETR */ yytestcase(yyruleno==98);
      /* (99) array_literal ::= BRACKETL element_list elision BRACKETR */ yytestcase(yyruleno==99);
      /* (100) array_literal ::= BRACKETL element_list BRACKETR */ yytestcase(yyruleno==100);
      /* (101) elision ::= COMMA */ yytestcase(yyruleno==101);
      /* (102) elision ::= elision COMMA */ yytestcase(yyruleno==102);
      /* (103) element_list ::= assignment_expression */ yytestcase(yyruleno==103);
      /* (104) element_list ::= elision assignment_expression */ yytestcase(yyruleno==104);
      /* (105) element_list ::= element_list elision assignment_expression */ yytestcase(yyruleno==105);
      /* (106) object_literal ::= BRACEL property_name_and_value_list BRACER */ yytestcase(yyruleno==106);
      /* (107) property_name_and_value_list ::= */ yytestcase(yyruleno==107);
      /* (108) property_name_and_value_list ::= property_name_and_value_list COMMA property_name_and_value */ yytestcase(yyruleno==108);
      /* (109) property_name_and_value ::= property_name COLON assignment_expression */ yytestcase(yyruleno==109);
      /* (110) property_name ::= IDENTIFIER|STRING|DECIMAL */ yytestcase(yyruleno==110);
      /* (111) member_expression_part ::= BRACKETL expression BRACKETR */ yytestcase(yyruleno==111);
      /* (112) member_expression_part ::= DOT IDENTIFIER */ yytestcase(yyruleno==112);
        break;
  };
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact < YYNSTATE ){
#ifdef NDEBUG
    /* If we are not debugging and the reduce action popped at least
    ** one element off the stack, then we can push the new element back
    ** onto the stack here, and skip the stack overflow test in yy_shift().
    ** That gives a significant speed improvement. */
    if( yysize ){
      yypParser->yyidx++;
      yymsp -= yysize-1;
      yymsp->stateno = (YYACTIONTYPE)yyact;
      yymsp->major = (YYCODETYPE)yygoto;
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
#ifndef YYNOERRORRECOVERY
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
#endif /* YYNOERRORRECOVERY */

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
    grn_obj buf;
    if (ctx->rc == GRN_SUCCESS) {
      GRN_TEXT_INIT(&buf, 0);
      GRN_TEXT_PUT(ctx, &buf, efsi->str, efsi->str_end - efsi->str);
      GRN_TEXT_PUTC(ctx, &buf, '\0');
      ERR(GRN_SYNTAX_ERROR, "Syntax error! (%s)", GRN_TEXT_VALUE(&buf));
      GRN_OBJ_FIN(ctx, &buf);
    }
  }
#line 1884 "expr.c"
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
    yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
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
        yy_destructor(yypParser, (YYCODETYPE)yymajor,&yyminorunion);
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
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
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
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor,yyminorunion);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      yymajor = YYNOCODE;
      
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
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
  return;
}
