/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is included that follows the "include" declaration
** in the input grammar file. */
#include <stdio.h>
#line 3 "ecmascript.y"

#define assert GRN_ASSERT
#line 11 "ecmascript.c"
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
#define YY_ACTTAB_COUNT (1683)
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
 /*   260 */   190,  176,   70,  104,    8,  169,  165,  107,  148,  170,
 /*   270 */   173,  194,   80,  115,  114,  123,  122,  121,  109,   81,
 /*   280 */    94,  105,   95,  159,  190,  176,   70,  145,    5,  169,
 /*   290 */   165,  166,   79,   21,  202,  148,  199,  323,  170,  143,
 /*   300 */   194,   80,  115,  114,  123,  122,  121,  109,   81,   94,
 /*   310 */   105,   95,  159,  190,  176,   70,  205,    5,  169,  165,
 /*   320 */   166,   79,  198,  197,  149,  103,  130,  203,   26,  323,
 /*   330 */   323,   62,   61,  323,  323,  323,   65,   64,   63,   60,
 /*   340 */    59,   58,   57,   56,   55,  164,  163,  162,  161,  160,
 /*   350 */     2,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*   360 */    62,   61,  323,  323,  323,   65,   64,   63,   60,   59,
 /*   370 */    58,   57,   56,   55,  164,  163,  162,  161,  160,    2,
 /*   380 */   170,  195,  194,   80,  115,  114,  123,  122,  121,  109,
 /*   390 */    81,   94,  105,   95,  159,  190,  176,   70,  323,  323,
 /*   400 */   169,  165,  323,  323,  323,  323,  323,  170,  193,  194,
 /*   410 */    80,  115,  114,  123,  122,  121,  109,   81,   94,  105,
 /*   420 */    95,  159,  190,  176,   70,  323,  323,  169,  165,  323,
 /*   430 */   323,  323,  323,  323,  323,  323,  170,  127,  194,   80,
 /*   440 */   115,  114,  123,  122,  121,  109,   81,   94,  105,   95,
 /*   450 */   159,  190,  176,   70,  323,  323,  169,  165,  170,  192,
 /*   460 */   194,   80,  115,  114,  123,  122,  121,  109,   81,   94,
 /*   470 */   105,   95,  159,  190,  176,   70,  323,  323,  169,  165,
 /*   480 */   323,  323,  323,  170,  153,  194,   80,  115,  114,  123,
 /*   490 */   122,  121,  109,   81,   94,  105,   95,  159,  190,  176,
 /*   500 */    70,  323,  323,  169,  165,  323,  323,  323,  323,  323,
 /*   510 */   323,  323,  170,  144,  194,   80,  115,  114,  123,  122,
 /*   520 */   121,  109,   81,   94,  105,   95,  159,  190,  176,   70,
 /*   530 */   323,  323,  169,  165,  170,  142,  194,   80,  115,  114,
 /*   540 */   123,  122,  121,  109,   81,   94,  105,   95,  159,  190,
 /*   550 */   176,   70,  323,  323,  169,  165,  323,  323,  323,  170,
 /*   560 */   141,  194,   80,  115,  114,  123,  122,  121,  109,   81,
 /*   570 */    94,  105,   95,  159,  190,  176,   70,  323,  323,  169,
 /*   580 */   165,  323,  323,  323,  323,  323,  323,  323,  170,  140,
 /*   590 */   194,   80,  115,  114,  123,  122,  121,  109,   81,   94,
 /*   600 */   105,   95,  159,  190,  176,   70,  323,  323,  169,  165,
 /*   610 */   170,  139,  194,   80,  115,  114,  123,  122,  121,  109,
 /*   620 */    81,   94,  105,   95,  159,  190,  176,   70,  323,  323,
 /*   630 */   169,  165,  323,  323,  323,  170,  138,  194,   80,  115,
 /*   640 */   114,  123,  122,  121,  109,   81,   94,  105,   95,  159,
 /*   650 */   190,  176,   70,  323,  323,  169,  165,  323,  323,  323,
 /*   660 */   323,  323,  323,  323,  170,  137,  194,   80,  115,  114,
 /*   670 */   123,  122,  121,  109,   81,   94,  105,   95,  159,  190,
 /*   680 */   176,   70,  323,  323,  169,  165,  170,  136,  194,   80,
 /*   690 */   115,  114,  123,  122,  121,  109,   81,   94,  105,   95,
 /*   700 */   159,  190,  176,   70,  323,  323,  169,  165,  323,  323,
 /*   710 */   323,  170,  135,  194,   80,  115,  114,  123,  122,  121,
 /*   720 */   109,   81,   94,  105,   95,  159,  190,  176,   70,  323,
 /*   730 */   323,  169,  165,  323,  323,  323,  323,  323,  323,  323,
 /*   740 */   170,  134,  194,   80,  115,  114,  123,  122,  121,  109,
 /*   750 */    81,   94,  105,   95,  159,  190,  176,   70,  323,  323,
 /*   760 */   169,  165,  170,  133,  194,   80,  115,  114,  123,  122,
 /*   770 */   121,  109,   81,   94,  105,   95,  159,  190,  176,   70,
 /*   780 */   323,  323,  169,  165,  323,  323,  323,  170,  132,  194,
 /*   790 */    80,  115,  114,  123,  122,  121,  109,   81,   94,  105,
 /*   800 */    95,  159,  190,  176,   70,  323,  323,  169,  165,  323,
 /*   810 */   323,  323,  323,  323,  323,  323,  170,  156,  194,   80,
 /*   820 */   115,  114,  123,  122,  121,  109,   81,   94,  105,   95,
 /*   830 */   159,  190,  176,   70,  323,  323,  169,  165,  170,  150,
 /*   840 */   194,   80,  115,  114,  123,  122,  121,  109,   81,   94,
 /*   850 */   105,   95,  159,  190,  176,   70,  323,  323,  169,  165,
 /*   860 */   323,  323,  323,  170,  323,  323,  111,  323,  102,  123,
 /*   870 */   122,  121,  109,   81,   94,  105,   95,  159,  190,  176,
 /*   880 */    70,  323,  323,  169,  165,  323,  323,  323,  323,  323,
 /*   890 */   323,  323,  170,  323,  323,  111,  323,  323,  126,  122,
 /*   900 */   121,  109,   81,   94,  105,   95,  159,  190,  176,   70,
 /*   910 */   323,  323,  169,  165,  170,  323,  323,  111,  323,  323,
 /*   920 */   118,  122,  121,  109,   81,   94,  105,   95,  159,  190,
 /*   930 */   176,   70,  323,  323,  169,  165,  323,  323,  323,  170,
 /*   940 */   323,  323,  111,  323,  323,  323,  125,  121,  109,   81,
 /*   950 */    94,  105,   95,  159,  190,  176,   70,  323,  323,  169,
 /*   960 */   165,   25,   20,   19,   18,   17,   16,   15,   14,   13,
 /*   970 */    12,   11,   10,  323,  170,  323,  323,  111,  323,  323,
 /*   980 */   323,  323,  124,  109,   81,   94,  105,   95,  159,  190,
 /*   990 */   176,   70,  323,  323,  169,  165,  170,  323,  323,  111,
 /*  1000 */   323,  323,  178,  177,  323,  113,   81,   94,  105,   95,
 /*  1010 */   159,  190,  176,   70,  323,  170,  169,  165,  111,  323,
 /*  1020 */   323,  323,  323,  323,  323,   83,   94,  105,   95,  159,
 /*  1030 */   190,  176,   70,  323,  323,  169,  165,  170,  323,  323,
 /*  1040 */   111,  323,  323,  323,  323,  323,  323,   82,   94,  105,
 /*  1050 */    95,  159,  190,  176,   70,  323,  323,  169,  165,  170,
 /*  1060 */   323,  323,  111,  323,  323,  323,  323,  323,  323,  323,
 /*  1070 */   100,  105,   95,  159,  190,  176,   70,  323,  170,  169,
 /*  1080 */   165,  111,  323,  323,  323,  323,  323,  323,  323,   98,
 /*  1090 */   105,   95,  159,  190,  176,   70,  323,  323,  169,  165,
 /*  1100 */   323,  170,  323,  323,  111,  323,  323,  323,  323,  323,
 /*  1110 */   323,  323,   96,  105,   95,  159,  190,  176,   70,  323,
 /*  1120 */   323,  169,  165,  170,  323,  323,  111,  323,  323,  323,
 /*  1130 */   323,  323,  323,  323,   93,  105,   95,  159,  190,  176,
 /*  1140 */    70,  323,  323,  169,  165,  323,  170,  323,  323,  111,
 /*  1150 */   323,  323,  323,  323,  323,  323,  323,   92,  105,   95,
 /*  1160 */   159,  190,  176,   70,  323,  170,  169,  165,  111,  323,
 /*  1170 */   323,  323,  323,  323,  323,  323,   91,  105,   95,  159,
 /*  1180 */   190,  176,   70,  323,  323,  169,  165,  323,  170,  323,
 /*  1190 */   323,  111,  323,  323,  323,  323,  323,  323,  323,   90,
 /*  1200 */   105,   95,  159,  190,  176,   70,  323,  323,  169,  165,
 /*  1210 */   170,  323,  323,  111,  323,  323,  323,  323,  323,  323,
 /*  1220 */   323,   89,  105,   95,  159,  190,  176,   70,  323,  323,
 /*  1230 */   169,  165,  323,  170,  323,  323,  111,  323,  323,  323,
 /*  1240 */   323,  323,  323,  323,   88,  105,   95,  159,  190,  176,
 /*  1250 */    70,  323,  170,  169,  165,  111,  323,  323,  323,  323,
 /*  1260 */   323,  323,  323,   87,  105,   95,  159,  190,  176,   70,
 /*  1270 */   323,  323,  169,  165,  323,  170,  323,  323,  111,  323,
 /*  1280 */   323,  323,  323,  323,  323,  323,   86,  105,   95,  159,
 /*  1290 */   190,  176,   70,  323,  323,  169,  165,  170,  323,  323,
 /*  1300 */   111,  323,  323,  323,  323,  323,  323,  323,   85,  105,
 /*  1310 */    95,  159,  190,  176,   70,  323,  323,  169,  165,  323,
 /*  1320 */   170,  323,  323,  111,  323,  323,  323,  323,  323,  323,
 /*  1330 */   323,   84,  105,   95,  159,  190,  176,   70,  323,  323,
 /*  1340 */   169,  165,  204,   77,   76,   73,  201,   68,  323,   75,
 /*  1350 */   128,    7,  323,   67,  323,   77,   76,   73,  201,   68,
 /*  1360 */   129,   75,  128,    7,  170,   67,  323,  111,  323,  323,
 /*  1370 */   323,  323,  323,  323,  323,  323,  112,   95,  159,  190,
 /*  1380 */   176,   70,  323,  323,  169,  165,  170,    5,  323,  111,
 /*  1390 */   166,   79,  323,  323,  323,  323,  323,  323,  108,   95,
 /*  1400 */   159,  190,  176,   70,  323,  323,  169,  165,  170,  323,
 /*  1410 */   323,  111,  323,  323,  323,  323,  323,  323,  323,  323,
 /*  1420 */   106,   95,  159,  190,  176,   70,  323,  323,  169,  165,
 /*  1430 */   323,  323,  323,  323,  323,  323,  323,  170,  323,  323,
 /*  1440 */   111,  323,  323,  323,  164,  163,  162,  161,  160,    2,
 /*  1450 */    99,  159,  190,  176,   70,  323,  170,  169,  165,  111,
 /*  1460 */   323,  323,  323,  323,  323,  170,  323,  323,  111,   97,
 /*  1470 */   159,  190,  176,   70,  323,  170,  169,  165,  111,  191,
 /*  1480 */   190,  176,   70,  323,  170,  169,  165,  111,  323,  189,
 /*  1490 */   190,  176,   70,  323,  323,  169,  165,  323,  188,  190,
 /*  1500 */   176,   70,  323,  323,  169,  165,  323,  323,  323,  323,
 /*  1510 */   323,  323,  323,  170,  323,  323,  111,  323,  323,  323,
 /*  1520 */   323,  170,  323,  323,  111,  323,  323,  187,  190,  176,
 /*  1530 */    70,  323,  323,  169,  165,  186,  190,  176,   70,  323,
 /*  1540 */   323,  169,  165,  323,  170,  323,  323,  111,  323,  323,
 /*  1550 */   323,  323,  170,  323,  323,  111,  323,  323,  185,  190,
 /*  1560 */   176,   70,  323,  323,  169,  165,  184,  190,  176,   70,
 /*  1570 */   323,  170,  169,  165,  111,  323,  323,  323,  323,  323,
 /*  1580 */   170,  323,  323,  111,  323,  183,  190,  176,   70,  323,
 /*  1590 */   323,  169,  165,  323,  182,  190,  176,   70,  323,  323,
 /*  1600 */   169,  165,  323,  170,  323,  323,  111,  323,  323,  323,
 /*  1610 */   323,  170,  323,  323,  111,  323,  323,  181,  190,  176,
 /*  1620 */    70,  323,  323,  169,  165,  180,  190,  176,   70,  323,
 /*  1630 */   323,  169,  165,  323,  170,  323,  323,  111,  323,  323,
 /*  1640 */   323,  323,  170,  323,  323,  111,  323,  323,  179,  190,
 /*  1650 */   176,   70,  323,  323,  169,  165,  171,  190,  176,   70,
 /*  1660 */   323,  170,  169,  165,  111,  323,  323,  323,  323,  323,
 /*  1670 */   323,  323,  323,  323,  323,  167,  190,  176,   70,  323,
 /*  1680 */   323,  169,  165,
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
 /*   260 */    90,   91,   92,   99,   97,   95,   96,   73,   12,   75,
 /*   270 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   85,
 /*   280 */    86,   87,   88,   89,   90,   91,   92,    6,    5,   95,
 /*   290 */    96,    8,    9,   12,   74,   12,   74,  103,   75,   76,
 /*   300 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   310 */    87,   88,   89,   90,   91,   92,    0,    5,   95,   96,
 /*   320 */     8,    9,   74,   74,   68,  102,   74,   74,   12,  103,
 /*   330 */   103,   48,   49,  103,  103,  103,   53,   54,   55,   56,
 /*   340 */    57,   58,   59,   60,   61,   62,   63,   64,   65,   66,
 /*   350 */    67,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*   360 */    48,   49,  103,  103,  103,   53,   54,   55,   56,   57,
 /*   370 */    58,   59,   60,   61,   62,   63,   64,   65,   66,   67,
 /*   380 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   390 */    85,   86,   87,   88,   89,   90,   91,   92,  103,  103,
 /*   400 */    95,   96,  103,  103,  103,  103,  103,   75,   76,   77,
 /*   410 */    78,   79,   80,   81,   82,   83,   84,   85,   86,   87,
 /*   420 */    88,   89,   90,   91,   92,  103,  103,   95,   96,  103,
 /*   430 */   103,  103,  103,  103,  103,  103,   75,   76,   77,   78,
 /*   440 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   88,
 /*   450 */    89,   90,   91,   92,  103,  103,   95,   96,   75,   76,
 /*   460 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   470 */    87,   88,   89,   90,   91,   92,  103,  103,   95,   96,
 /*   480 */   103,  103,  103,   75,   76,   77,   78,   79,   80,   81,
 /*   490 */    82,   83,   84,   85,   86,   87,   88,   89,   90,   91,
 /*   500 */    92,  103,  103,   95,   96,  103,  103,  103,  103,  103,
 /*   510 */   103,  103,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   520 */    83,   84,   85,   86,   87,   88,   89,   90,   91,   92,
 /*   530 */   103,  103,   95,   96,   75,   76,   77,   78,   79,   80,
 /*   540 */    81,   82,   83,   84,   85,   86,   87,   88,   89,   90,
 /*   550 */    91,   92,  103,  103,   95,   96,  103,  103,  103,   75,
 /*   560 */    76,   77,   78,   79,   80,   81,   82,   83,   84,   85,
 /*   570 */    86,   87,   88,   89,   90,   91,   92,  103,  103,   95,
 /*   580 */    96,  103,  103,  103,  103,  103,  103,  103,   75,   76,
 /*   590 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   600 */    87,   88,   89,   90,   91,   92,  103,  103,   95,   96,
 /*   610 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   620 */    85,   86,   87,   88,   89,   90,   91,   92,  103,  103,
 /*   630 */    95,   96,  103,  103,  103,   75,   76,   77,   78,   79,
 /*   640 */    80,   81,   82,   83,   84,   85,   86,   87,   88,   89,
 /*   650 */    90,   91,   92,  103,  103,   95,   96,  103,  103,  103,
 /*   660 */   103,  103,  103,  103,   75,   76,   77,   78,   79,   80,
 /*   670 */    81,   82,   83,   84,   85,   86,   87,   88,   89,   90,
 /*   680 */    91,   92,  103,  103,   95,   96,   75,   76,   77,   78,
 /*   690 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   88,
 /*   700 */    89,   90,   91,   92,  103,  103,   95,   96,  103,  103,
 /*   710 */   103,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   720 */    84,   85,   86,   87,   88,   89,   90,   91,   92,  103,
 /*   730 */   103,   95,   96,  103,  103,  103,  103,  103,  103,  103,
 /*   740 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   750 */    85,   86,   87,   88,   89,   90,   91,   92,  103,  103,
 /*   760 */    95,   96,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   770 */    83,   84,   85,   86,   87,   88,   89,   90,   91,   92,
 /*   780 */   103,  103,   95,   96,  103,  103,  103,   75,   76,   77,
 /*   790 */    78,   79,   80,   81,   82,   83,   84,   85,   86,   87,
 /*   800 */    88,   89,   90,   91,   92,  103,  103,   95,   96,  103,
 /*   810 */   103,  103,  103,  103,  103,  103,   75,   76,   77,   78,
 /*   820 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   88,
 /*   830 */    89,   90,   91,   92,  103,  103,   95,   96,   75,   76,
 /*   840 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   850 */    87,   88,   89,   90,   91,   92,  103,  103,   95,   96,
 /*   860 */   103,  103,  103,   75,  103,  103,   78,  103,   80,   81,
 /*   870 */    82,   83,   84,   85,   86,   87,   88,   89,   90,   91,
 /*   880 */    92,  103,  103,   95,   96,  103,  103,  103,  103,  103,
 /*   890 */   103,  103,   75,  103,  103,   78,  103,  103,   81,   82,
 /*   900 */    83,   84,   85,   86,   87,   88,   89,   90,   91,   92,
 /*   910 */   103,  103,   95,   96,   75,  103,  103,   78,  103,  103,
 /*   920 */    81,   82,   83,   84,   85,   86,   87,   88,   89,   90,
 /*   930 */    91,   92,  103,  103,   95,   96,  103,  103,  103,   75,
 /*   940 */   103,  103,   78,  103,  103,  103,   82,   83,   84,   85,
 /*   950 */    86,   87,   88,   89,   90,   91,   92,  103,  103,   95,
 /*   960 */    96,   13,   14,   15,   16,   17,   18,   19,   20,   21,
 /*   970 */    22,   23,   24,  103,   75,  103,  103,   78,  103,  103,
 /*   980 */   103,  103,   83,   84,   85,   86,   87,   88,   89,   90,
 /*   990 */    91,   92,  103,  103,   95,   96,   75,  103,  103,   78,
 /*  1000 */   103,  103,   54,   55,  103,   84,   85,   86,   87,   88,
 /*  1010 */    89,   90,   91,   92,  103,   75,   95,   96,   78,  103,
 /*  1020 */   103,  103,  103,  103,  103,   85,   86,   87,   88,   89,
 /*  1030 */    90,   91,   92,  103,  103,   95,   96,   75,  103,  103,
 /*  1040 */    78,  103,  103,  103,  103,  103,  103,   85,   86,   87,
 /*  1050 */    88,   89,   90,   91,   92,  103,  103,   95,   96,   75,
 /*  1060 */   103,  103,   78,  103,  103,  103,  103,  103,  103,  103,
 /*  1070 */    86,   87,   88,   89,   90,   91,   92,  103,   75,   95,
 /*  1080 */    96,   78,  103,  103,  103,  103,  103,  103,  103,   86,
 /*  1090 */    87,   88,   89,   90,   91,   92,  103,  103,   95,   96,
 /*  1100 */   103,   75,  103,  103,   78,  103,  103,  103,  103,  103,
 /*  1110 */   103,  103,   86,   87,   88,   89,   90,   91,   92,  103,
 /*  1120 */   103,   95,   96,   75,  103,  103,   78,  103,  103,  103,
 /*  1130 */   103,  103,  103,  103,   86,   87,   88,   89,   90,   91,
 /*  1140 */    92,  103,  103,   95,   96,  103,   75,  103,  103,   78,
 /*  1150 */   103,  103,  103,  103,  103,  103,  103,   86,   87,   88,
 /*  1160 */    89,   90,   91,   92,  103,   75,   95,   96,   78,  103,
 /*  1170 */   103,  103,  103,  103,  103,  103,   86,   87,   88,   89,
 /*  1180 */    90,   91,   92,  103,  103,   95,   96,  103,   75,  103,
 /*  1190 */   103,   78,  103,  103,  103,  103,  103,  103,  103,   86,
 /*  1200 */    87,   88,   89,   90,   91,   92,  103,  103,   95,   96,
 /*  1210 */    75,  103,  103,   78,  103,  103,  103,  103,  103,  103,
 /*  1220 */   103,   86,   87,   88,   89,   90,   91,   92,  103,  103,
 /*  1230 */    95,   96,  103,   75,  103,  103,   78,  103,  103,  103,
 /*  1240 */   103,  103,  103,  103,   86,   87,   88,   89,   90,   91,
 /*  1250 */    92,  103,   75,   95,   96,   78,  103,  103,  103,  103,
 /*  1260 */   103,  103,  103,   86,   87,   88,   89,   90,   91,   92,
 /*  1270 */   103,  103,   95,   96,  103,   75,  103,  103,   78,  103,
 /*  1280 */   103,  103,  103,  103,  103,  103,   86,   87,   88,   89,
 /*  1290 */    90,   91,   92,  103,  103,   95,   96,   75,  103,  103,
 /*  1300 */    78,  103,  103,  103,  103,  103,  103,  103,   86,   87,
 /*  1310 */    88,   89,   90,   91,   92,  103,  103,   95,   96,  103,
 /*  1320 */    75,  103,  103,   78,  103,  103,  103,  103,  103,  103,
 /*  1330 */   103,   86,   87,   88,   89,   90,   91,   92,  103,  103,
 /*  1340 */    95,   96,    0,    1,    2,    3,    4,    5,  103,    7,
 /*  1350 */     8,    9,  103,   11,  103,    1,    2,    3,    4,    5,
 /*  1360 */     6,    7,    8,    9,   75,   11,  103,   78,  103,  103,
 /*  1370 */   103,  103,  103,  103,  103,  103,   87,   88,   89,   90,
 /*  1380 */    91,   92,  103,  103,   95,   96,   75,    5,  103,   78,
 /*  1390 */     8,    9,  103,  103,  103,  103,  103,  103,   87,   88,
 /*  1400 */    89,   90,   91,   92,  103,  103,   95,   96,   75,  103,
 /*  1410 */   103,   78,  103,  103,  103,  103,  103,  103,  103,  103,
 /*  1420 */    87,   88,   89,   90,   91,   92,  103,  103,   95,   96,
 /*  1430 */   103,  103,  103,  103,  103,  103,  103,   75,  103,  103,
 /*  1440 */    78,  103,  103,  103,   62,   63,   64,   65,   66,   67,
 /*  1450 */    88,   89,   90,   91,   92,  103,   75,   95,   96,   78,
 /*  1460 */   103,  103,  103,  103,  103,   75,  103,  103,   78,   88,
 /*  1470 */    89,   90,   91,   92,  103,   75,   95,   96,   78,   89,
 /*  1480 */    90,   91,   92,  103,   75,   95,   96,   78,  103,   89,
 /*  1490 */    90,   91,   92,  103,  103,   95,   96,  103,   89,   90,
 /*  1500 */    91,   92,  103,  103,   95,   96,  103,  103,  103,  103,
 /*  1510 */   103,  103,  103,   75,  103,  103,   78,  103,  103,  103,
 /*  1520 */   103,   75,  103,  103,   78,  103,  103,   89,   90,   91,
 /*  1530 */    92,  103,  103,   95,   96,   89,   90,   91,   92,  103,
 /*  1540 */   103,   95,   96,  103,   75,  103,  103,   78,  103,  103,
 /*  1550 */   103,  103,   75,  103,  103,   78,  103,  103,   89,   90,
 /*  1560 */    91,   92,  103,  103,   95,   96,   89,   90,   91,   92,
 /*  1570 */   103,   75,   95,   96,   78,  103,  103,  103,  103,  103,
 /*  1580 */    75,  103,  103,   78,  103,   89,   90,   91,   92,  103,
 /*  1590 */   103,   95,   96,  103,   89,   90,   91,   92,  103,  103,
 /*  1600 */    95,   96,  103,   75,  103,  103,   78,  103,  103,  103,
 /*  1610 */   103,   75,  103,  103,   78,  103,  103,   89,   90,   91,
 /*  1620 */    92,  103,  103,   95,   96,   89,   90,   91,   92,  103,
 /*  1630 */   103,   95,   96,  103,   75,  103,  103,   78,  103,  103,
 /*  1640 */   103,  103,   75,  103,  103,   78,  103,  103,   89,   90,
 /*  1650 */    91,   92,  103,  103,   95,   96,   89,   90,   91,   92,
 /*  1660 */   103,   75,   95,   96,   78,  103,  103,  103,  103,  103,
 /*  1670 */   103,  103,  103,  103,  103,   89,   90,   91,   92,  103,
 /*  1680 */   103,   95,   96,
};
#define YY_SHIFT_USE_DFLT (-1)
#define YY_SHIFT_COUNT (128)
#define YY_SHIFT_MIN   (0)
#define YY_SHIFT_MAX   (1382)
static const short yy_shift_ofst[] = {
 /*     0 */    47,   47,  283,  312,  312,  312,  312,  312,  174,  110,
 /*    10 */   312,  312,  312,  312,  312,  312,  312,  312,  312,  312,
 /*    20 */   312,  312,  312,  312,  312,  312,  312,  312,  312,  312,
 /*    30 */   312,  312,  312,  312,  312,  312,  312,  312,  312,  312,
 /*    40 */   312,  312,  312,  312,  312,  312,  312,  312,  312,  312,
 /*    50 */   312,  312,  312,  312,  312,  312,  312,  312,  312,  312,
 /*    60 */   312,  312,  312,  312,  312,  312,  312, 1382,   77,   90,
 /*    70 */    82, 1354, 1342,   77,   77,   77,   77,   77,  256,   -1,
 /*    80 */   948,  177,  177,  177,   44,   44,   44,   44,   44,   44,
 /*    90 */    44,   44,   44,   44,   44,   42,   44,   42,   44,   42,
 /*   100 */    44,  316,   21,  281,  111,   29,   29,  149,   29,   17,
 /*   110 */   138,   62,   29,   17,   21,  159,   87,   46,   97,  236,
 /*   120 */   199,  176,  125,   97,  176,  125,   97,   94,   46,
};
#define YY_REDUCE_USE_DFLT (-72)
#define YY_REDUCE_COUNT (79)
#define YY_REDUCE_MIN   (-71)
#define YY_REDUCE_MAX   (1586)
static const short yy_reduce_ofst[] = {
 /*     0 */   -71,  -46,   50,  -16,  223,  194,  170,  112,  763,  741,
 /*    10 */   712,  687,  665,  636,  611,  589,  560,  535,  513,  484,
 /*    20 */   459,  437,  408,  383,  361,  332,  305,  788,  839,  817,
 /*    30 */   864,  899,  921,  962,  940, 1245, 1222, 1200, 1177, 1158,
 /*    40 */  1135, 1113, 1090, 1071, 1048, 1026, 1003,  984, 1333, 1311,
 /*    50 */  1289, 1381, 1362, 1586, 1567, 1559, 1536, 1528, 1505, 1496,
 /*    60 */  1477, 1469, 1446, 1438, 1409, 1400, 1390,   85,  152,   43,
 /*    70 */    63,  253,  253,  252,  249,  248,  222,  220,  167,  164,
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
  "$",             "LOGICAL_AND",   "LOGICAL_AND_NOT",  "LOGICAL_OR",  
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
 /*   5 */ "query ::= query LOGICAL_AND_NOT query_element",
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
 /*  34 */ "logical_and_expression ::= logical_and_expression LOGICAL_AND_NOT bitwise_or_expression",
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
#line 29 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, grn_int32_value_at(&efsi->op_stack, -1), 2);
}
#line 1275 "ecmascript.c"
        break;
      case 4: /* query ::= query LOGICAL_AND query_element */
      case 33: /* logical_and_expression ::= logical_and_expression LOGICAL_AND bitwise_or_expression */ yytestcase(yyruleno==33);
#line 32 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND, 2);
}
#line 1283 "ecmascript.c"
        break;
      case 5: /* query ::= query LOGICAL_AND_NOT query_element */
      case 34: /* logical_and_expression ::= logical_and_expression LOGICAL_AND_NOT bitwise_or_expression */ yytestcase(yyruleno==34);
#line 35 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND_NOT, 2);
}
#line 1291 "ecmascript.c"
        break;
      case 6: /* query ::= query LOGICAL_OR query_element */
      case 31: /* logical_or_expression ::= logical_or_expression LOGICAL_OR logical_and_expression */ yytestcase(yyruleno==31);
#line 38 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_OR, 2);
}
#line 1299 "ecmascript.c"
        break;
      case 9: /* query_element ::= RELATIVE_OP query_element */
#line 45 "ecmascript.y"
{
  int mode;
  GRN_UINT32_POP(&efsi->mode_stack, mode);
}
#line 1307 "ecmascript.c"
        break;
      case 10: /* query_element ::= IDENTIFIER RELATIVE_OP query_element */
#line 49 "ecmascript.y"
{
  int mode;
  grn_obj *c;
  GRN_PTR_POP(&efsi->column_stack, c);
  GRN_UINT32_POP(&efsi->mode_stack, mode);
}
#line 1317 "ecmascript.c"
        break;
      case 11: /* query_element ::= BRACEL expression BRACER */
      case 12: /* query_element ::= EVAL primary_expression */ yytestcase(yyruleno==12);
#line 55 "ecmascript.y"
{
  efsi->flags = efsi->default_flags;
}
#line 1325 "ecmascript.c"
        break;
      case 14: /* expression ::= expression COMMA assignment_expression */
#line 63 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_COMMA, 2);
}
#line 1332 "ecmascript.c"
        break;
      case 16: /* assignment_expression ::= lefthand_side_expression ASSIGN assignment_expression */
#line 68 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_ASSIGN, 2);
}
#line 1339 "ecmascript.c"
        break;
      case 17: /* assignment_expression ::= lefthand_side_expression STAR_ASSIGN assignment_expression */
#line 71 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_STAR_ASSIGN, 2);
}
#line 1346 "ecmascript.c"
        break;
      case 18: /* assignment_expression ::= lefthand_side_expression SLASH_ASSIGN assignment_expression */
#line 74 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SLASH_ASSIGN, 2);
}
#line 1353 "ecmascript.c"
        break;
      case 19: /* assignment_expression ::= lefthand_side_expression MOD_ASSIGN assignment_expression */
#line 77 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MOD_ASSIGN, 2);
}
#line 1360 "ecmascript.c"
        break;
      case 20: /* assignment_expression ::= lefthand_side_expression PLUS_ASSIGN assignment_expression */
#line 80 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PLUS_ASSIGN, 2);
}
#line 1367 "ecmascript.c"
        break;
      case 21: /* assignment_expression ::= lefthand_side_expression MINUS_ASSIGN assignment_expression */
#line 83 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MINUS_ASSIGN, 2);
}
#line 1374 "ecmascript.c"
        break;
      case 22: /* assignment_expression ::= lefthand_side_expression SHIFTL_ASSIGN assignment_expression */
#line 86 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTL_ASSIGN, 2);
}
#line 1381 "ecmascript.c"
        break;
      case 23: /* assignment_expression ::= lefthand_side_expression SHIFTR_ASSIGN assignment_expression */
#line 89 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTR_ASSIGN, 2);
}
#line 1388 "ecmascript.c"
        break;
      case 24: /* assignment_expression ::= lefthand_side_expression SHIFTRR_ASSIGN assignment_expression */
#line 92 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTRR_ASSIGN, 2);
}
#line 1395 "ecmascript.c"
        break;
      case 25: /* assignment_expression ::= lefthand_side_expression AND_ASSIGN assignment_expression */
#line 95 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND_ASSIGN, 2);
}
#line 1402 "ecmascript.c"
        break;
      case 26: /* assignment_expression ::= lefthand_side_expression XOR_ASSIGN assignment_expression */
#line 98 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_XOR_ASSIGN, 2);
}
#line 1409 "ecmascript.c"
        break;
      case 27: /* assignment_expression ::= lefthand_side_expression OR_ASSIGN assignment_expression */
#line 101 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_OR_ASSIGN, 2);
}
#line 1416 "ecmascript.c"
        break;
      case 29: /* conditional_expression ::= logical_or_expression QUESTION assignment_expression COLON assignment_expression */
#line 106 "ecmascript.y"
{
  grn_expr *e = (grn_expr *)efsi->e;
  e->codes[yymsp[-3].minor.yy0].nargs = yymsp[-1].minor.yy0 - yymsp[-3].minor.yy0;
  e->codes[yymsp[-1].minor.yy0].nargs = e->codes_curr - yymsp[-1].minor.yy0 - 1;
}
#line 1425 "ecmascript.c"
        break;
      case 36: /* bitwise_or_expression ::= bitwise_or_expression BITWISE_OR bitwise_xor_expression */
#line 126 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_OR, 2);
}
#line 1432 "ecmascript.c"
        break;
      case 38: /* bitwise_xor_expression ::= bitwise_xor_expression BITWISE_XOR bitwise_and_expression */
#line 131 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_XOR, 2);
}
#line 1439 "ecmascript.c"
        break;
      case 40: /* bitwise_and_expression ::= bitwise_and_expression BITWISE_AND equality_expression */
#line 136 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_AND, 2);
}
#line 1446 "ecmascript.c"
        break;
      case 42: /* equality_expression ::= equality_expression EQUAL relational_expression */
#line 141 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_EQUAL, 2);
}
#line 1453 "ecmascript.c"
        break;
      case 43: /* equality_expression ::= equality_expression NOT_EQUAL relational_expression */
#line 144 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NOT_EQUAL, 2);
}
#line 1460 "ecmascript.c"
        break;
      case 45: /* relational_expression ::= relational_expression LESS shift_expression */
#line 149 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_LESS, 2);
}
#line 1467 "ecmascript.c"
        break;
      case 46: /* relational_expression ::= relational_expression GREATER shift_expression */
#line 152 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_GREATER, 2);
}
#line 1474 "ecmascript.c"
        break;
      case 47: /* relational_expression ::= relational_expression LESS_EQUAL shift_expression */
#line 155 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_LESS_EQUAL, 2);
}
#line 1481 "ecmascript.c"
        break;
      case 48: /* relational_expression ::= relational_expression GREATER_EQUAL shift_expression */
#line 158 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_GREATER_EQUAL, 2);
}
#line 1488 "ecmascript.c"
        break;
      case 49: /* relational_expression ::= relational_expression IN shift_expression */
#line 161 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_IN, 2);
}
#line 1495 "ecmascript.c"
        break;
      case 50: /* relational_expression ::= relational_expression MATCH shift_expression */
#line 164 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MATCH, 2);
}
#line 1502 "ecmascript.c"
        break;
      case 51: /* relational_expression ::= relational_expression NEAR shift_expression */
#line 167 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NEAR, 2);
}
#line 1509 "ecmascript.c"
        break;
      case 52: /* relational_expression ::= relational_expression NEAR2 shift_expression */
#line 170 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NEAR2, 2);
}
#line 1516 "ecmascript.c"
        break;
      case 53: /* relational_expression ::= relational_expression SIMILAR shift_expression */
#line 173 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SIMILAR, 2);
}
#line 1523 "ecmascript.c"
        break;
      case 54: /* relational_expression ::= relational_expression TERM_EXTRACT shift_expression */
#line 176 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_TERM_EXTRACT, 2);
}
#line 1530 "ecmascript.c"
        break;
      case 55: /* relational_expression ::= relational_expression LCP shift_expression */
#line 179 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_LCP, 2);
}
#line 1537 "ecmascript.c"
        break;
      case 56: /* relational_expression ::= relational_expression PREFIX shift_expression */
#line 182 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PREFIX, 2);
}
#line 1544 "ecmascript.c"
        break;
      case 57: /* relational_expression ::= relational_expression SUFFIX shift_expression */
#line 185 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SUFFIX, 2);
}
#line 1551 "ecmascript.c"
        break;
      case 59: /* shift_expression ::= shift_expression SHIFTL additive_expression */
#line 190 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTL, 2);
}
#line 1558 "ecmascript.c"
        break;
      case 60: /* shift_expression ::= shift_expression SHIFTR additive_expression */
#line 193 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTR, 2);
}
#line 1565 "ecmascript.c"
        break;
      case 61: /* shift_expression ::= shift_expression SHIFTRR additive_expression */
#line 196 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTRR, 2);
}
#line 1572 "ecmascript.c"
        break;
      case 63: /* additive_expression ::= additive_expression PLUS multiplicative_expression */
#line 201 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PLUS, 2);
}
#line 1579 "ecmascript.c"
        break;
      case 64: /* additive_expression ::= additive_expression MINUS multiplicative_expression */
#line 204 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MINUS, 2);
}
#line 1586 "ecmascript.c"
        break;
      case 66: /* multiplicative_expression ::= multiplicative_expression STAR unary_expression */
#line 209 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_STAR, 2);
}
#line 1593 "ecmascript.c"
        break;
      case 67: /* multiplicative_expression ::= multiplicative_expression SLASH unary_expression */
#line 212 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SLASH, 2);
}
#line 1600 "ecmascript.c"
        break;
      case 68: /* multiplicative_expression ::= multiplicative_expression MOD unary_expression */
#line 215 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MOD, 2);
}
#line 1607 "ecmascript.c"
        break;
      case 70: /* unary_expression ::= DELETE unary_expression */
#line 220 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_DELETE, 1);
}
#line 1614 "ecmascript.c"
        break;
      case 71: /* unary_expression ::= INCR unary_expression */
#line 223 "ecmascript.y"
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
        "constant can't be incremented (%.*s)",
        (int)(efsi->str_end - efsi->str), efsi->str);
  } else {
    grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_INCR, 1);
  }
}
#line 1635 "ecmascript.c"
        break;
      case 72: /* unary_expression ::= DECR unary_expression */
#line 240 "ecmascript.y"
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
        "constant can't be decremented (%.*s)",
        (int)(efsi->str_end - efsi->str), efsi->str);
  } else {
    grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_DECR, 1);
  }
}
#line 1656 "ecmascript.c"
        break;
      case 73: /* unary_expression ::= PLUS unary_expression */
#line 257 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PLUS, 1);
}
#line 1663 "ecmascript.c"
        break;
      case 74: /* unary_expression ::= MINUS unary_expression */
#line 260 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MINUS, 1);
}
#line 1670 "ecmascript.c"
        break;
      case 75: /* unary_expression ::= NOT unary_expression */
#line 263 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NOT, 1);
}
#line 1677 "ecmascript.c"
        break;
      case 76: /* unary_expression ::= BITWISE_NOT unary_expression */
#line 266 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_NOT, 1);
}
#line 1684 "ecmascript.c"
        break;
      case 77: /* unary_expression ::= ADJUST unary_expression */
#line 269 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_ADJUST, 1);
}
#line 1691 "ecmascript.c"
        break;
      case 78: /* unary_expression ::= EXACT unary_expression */
#line 272 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_EXACT, 1);
}
#line 1698 "ecmascript.c"
        break;
      case 79: /* unary_expression ::= PARTIAL unary_expression */
#line 275 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PARTIAL, 1);
}
#line 1705 "ecmascript.c"
        break;
      case 80: /* unary_expression ::= UNSPLIT unary_expression */
#line 278 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_UNSPLIT, 1);
}
#line 1712 "ecmascript.c"
        break;
      case 82: /* postfix_expression ::= lefthand_side_expression INCR */
#line 283 "ecmascript.y"
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
        "constant can't be incremented (%.*s)",
        (int)(efsi->str_end - efsi->str), efsi->str);
  } else {
    grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_INCR_POST, 1);
  }
}
#line 1733 "ecmascript.c"
        break;
      case 83: /* postfix_expression ::= lefthand_side_expression DECR */
#line 300 "ecmascript.y"
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
        "constant can't be decremented (%.*s)",
        (int)(efsi->str_end - efsi->str), efsi->str);
  } else {
    grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_DECR_POST, 1);
  }
}
#line 1754 "ecmascript.c"
        break;
      case 86: /* call_expression ::= member_expression arguments */
#line 321 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_CALL, yymsp[0].minor.yy0);
}
#line 1761 "ecmascript.c"
        break;
      case 113: /* arguments ::= PARENL argument_list PARENR */
#line 360 "ecmascript.y"
{ yygotominor.yy0 = yymsp[-1].minor.yy0; }
#line 1766 "ecmascript.c"
        break;
      case 114: /* argument_list ::= */
#line 361 "ecmascript.y"
{ yygotominor.yy0 = 0; }
#line 1771 "ecmascript.c"
        break;
      case 115: /* argument_list ::= assignment_expression */
#line 362 "ecmascript.y"
{ yygotominor.yy0 = 1; }
#line 1776 "ecmascript.c"
        break;
      case 116: /* argument_list ::= argument_list COMMA assignment_expression */
#line 363 "ecmascript.y"
{ yygotominor.yy0 = yymsp[-2].minor.yy0 + 1; }
#line 1781 "ecmascript.c"
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
#line 11 "ecmascript.y"

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
#line 1904 "ecmascript.c"
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
