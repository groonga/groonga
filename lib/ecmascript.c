/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is include which follows the "include" declaration
** in the input file. */
#include <stdio.h>
#line 3 "ecmascript.y"

#define assert GRN_ASSERT
#line 13 "ecmascript.c"
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
 /*     0 */   322,   71,  116,  132,  162,  159,  138,   80,  102,  103,
 /*    10 */   122,  123,  124,  108,   83,   90,  112,   89,  173,  142,
 /*    20 */   156,   69,  178,  125,  163,  167,  131,    1,  129,   75,
 /*    30 */   128,    3,  130,   67,   72,  110,  132,  162,  159,  138,
 /*    40 */    80,  102,  103,  122,  123,  124,  108,   83,   90,  112,
 /*    50 */    89,  173,  142,  156,   69,   28,   29,  163,  167,   73,
 /*    60 */    74,   77,  131,   68,  203,   75,  117,    4,  133,   67,
 /*    70 */    57,   58,   48,   49,   50,   54,   55,   56,   59,   60,
 /*    80 */    61,   62,   63,   64,  168,  169,  170,  171,  172,    2,
 /*    90 */   162,  185,  138,   80,  102,  103,  122,  123,  124,  108,
 /*   100 */    83,   90,  112,   89,  173,  142,  156,   69,  184,   10,
 /*   110 */   163,  167,    8,   78,  101,    9,  162,  159,  138,   80,
 /*   120 */   102,  103,  122,  123,  124,  108,   83,   90,  112,   89,
 /*   130 */   173,  142,  156,   69,  134,    6,  163,  167,  166,   79,
 /*   140 */   113,  136,  175,   10,  101,  113,  162,  159,  138,   80,
 /*   150 */   102,  103,  122,  123,  124,  108,   83,   90,  112,   89,
 /*   160 */   173,  142,  156,   69,  183,  160,  163,  167,  131,   68,
 /*   170 */    76,   75,  117,    4,   72,   67,  132,  135,   57,   58,
 /*   180 */   201,  157,  158,   54,   55,   56,   59,   60,   61,   62,
 /*   190 */    63,   64,  168,  169,  170,  171,  172,    2,  174,    6,
 /*   200 */   163,  167,  166,   79,   27,  107,  175,  162,  159,  138,
 /*   210 */    80,  102,  103,  122,  123,  124,  108,   83,   90,  112,
 /*   220 */    89,  173,  142,  156,   69,  202,   12,  163,  167,   35,
 /*   230 */    36,   37,   38,   39,   40,   41,   42,   43,   44,   45,
 /*   240 */    46,   47,   57,   58,   33,   34,    7,   54,   55,   56,
 /*   250 */    59,   60,   61,   62,   63,   64,  168,  169,  170,  171,
 /*   260 */   172,    2,  181,    6,   51,   52,  166,   79,   13,  110,
 /*   270 */   184,  162,  159,  138,   80,  102,  103,  122,  123,  124,
 /*   280 */   108,   83,   90,  112,   89,  173,  142,  156,   69,  154,
 /*   290 */   155,  163,  167,  204,   73,   74,   77,  131,   68,   30,
 /*   300 */    75,  117,    4,  164,   67,   31,   57,   58,    5,   10,
 /*   310 */   126,   54,   55,   56,   59,   60,   61,   62,   63,   64,
 /*   320 */   168,  169,  170,  171,  172,    2,  162,  189,  138,   80,
 /*   330 */   102,  103,  122,  123,  124,  108,   83,   90,  112,   89,
 /*   340 */   173,  142,  156,   69,  187,    6,  163,  167,  166,   79,
 /*   350 */    15,   32,  186,  114,  162,  176,  138,   80,  102,  103,
 /*   360 */   122,  123,  124,  108,   83,   90,  112,   89,  173,  142,
 /*   370 */   156,   69,  205,   14,  163,  167,   53,   65,   66,  177,
 /*   380 */   323,   70,  323,  323,   10,  323,  323,  323,   57,   58,
 /*   390 */   323,  323,  323,   54,   55,   56,   59,   60,   61,   62,
 /*   400 */    63,   64,  168,  169,  170,  171,  172,    2,  162,  182,
 /*   410 */   138,   80,  102,  103,  122,  123,  124,  108,   83,   90,
 /*   420 */   112,   89,  173,  142,  156,   69,  323,  323,  163,  167,
 /*   430 */   162,  137,  138,   80,  102,  103,  122,  123,  124,  108,
 /*   440 */    83,   90,  112,   89,  173,  142,  156,   69,  323,  323,
 /*   450 */   163,  167,  323,  323,  323,  323,  323,  323,  323,  323,
 /*   460 */   323,  323,  323,  323,  162,  139,  138,   80,  102,  103,
 /*   470 */   122,  123,  124,  108,   83,   90,  112,   89,  173,  142,
 /*   480 */   156,   69,  180,  323,  163,  167,  162,  118,  138,   80,
 /*   490 */   102,  103,  122,  123,  124,  108,   83,   90,  112,   89,
 /*   500 */   173,  142,  156,   69,  323,  323,  163,  167,  162,  140,
 /*   510 */   138,   80,  102,  103,  122,  123,  124,  108,   83,   90,
 /*   520 */   112,   89,  173,  142,  156,   69,  323,  323,  163,  167,
 /*   530 */   323,  323,  323,  323,  323,  323,  180,  323,  180,  323,
 /*   540 */   162,  179,  138,   80,  102,  103,  122,  123,  124,  108,
 /*   550 */    83,   90,  112,   89,  173,  142,  156,   69,  323,  323,
 /*   560 */   163,  167,  162,  188,  138,   80,  102,  103,  122,  123,
 /*   570 */   124,  108,   83,   90,  112,   89,  173,  142,  156,   69,
 /*   580 */   323,  323,  163,  167,  162,  190,  138,   80,  102,  103,
 /*   590 */   122,  123,  124,  108,   83,   90,  112,   89,  173,  142,
 /*   600 */   156,   69,  323,  323,  163,  167,  323,  323,  323,  323,
 /*   610 */   323,  323,  323,  323,  323,  323,  162,  191,  138,   80,
 /*   620 */   102,  103,  122,  123,  124,  108,   83,   90,  112,   89,
 /*   630 */   173,  142,  156,   69,  323,  323,  163,  167,  162,  192,
 /*   640 */   138,   80,  102,  103,  122,  123,  124,  108,   83,   90,
 /*   650 */   112,   89,  173,  142,  156,   69,  323,  323,  163,  167,
 /*   660 */   162,  193,  138,   80,  102,  103,  122,  123,  124,  108,
 /*   670 */    83,   90,  112,   89,  173,  142,  156,   69,  323,  323,
 /*   680 */   163,  167,  323,  323,  323,  323,  323,  323,  323,  323,
 /*   690 */   323,  323,  162,  194,  138,   80,  102,  103,  122,  123,
 /*   700 */   124,  108,   83,   90,  112,   89,  173,  142,  156,   69,
 /*   710 */   323,  323,  163,  167,  162,  195,  138,   80,  102,  103,
 /*   720 */   122,  123,  124,  108,   83,   90,  112,   89,  173,  142,
 /*   730 */   156,   69,  323,  323,  163,  167,  162,  196,  138,   80,
 /*   740 */   102,  103,  122,  123,  124,  108,   83,   90,  112,   89,
 /*   750 */   173,  142,  156,   69,  323,  323,  163,  167,  323,  323,
 /*   760 */   323,  323,  323,  323,  323,  323,  323,  323,  162,  197,
 /*   770 */   138,   80,  102,  103,  122,  123,  124,  108,   83,   90,
 /*   780 */   112,   89,  173,  142,  156,   69,  323,  323,  163,  167,
 /*   790 */   162,  198,  138,   80,  102,  103,  122,  123,  124,  108,
 /*   800 */    83,   90,  112,   89,  173,  142,  156,   69,  323,  323,
 /*   810 */   163,  167,  162,  199,  138,   80,  102,  103,  122,  123,
 /*   820 */   124,  108,   83,   90,  112,   89,  173,  142,  156,   69,
 /*   830 */   323,  323,  163,  167,  323,  323,  323,  323,  323,  323,
 /*   840 */   323,  323,  323,  323,  162,  200,  138,   80,  102,  103,
 /*   850 */   122,  123,  124,  108,   83,   90,  112,   89,  173,  142,
 /*   860 */   156,   69,  323,  323,  163,  167,  162,  323,  323,  106,
 /*   870 */   323,  115,  122,  123,  124,  108,   83,   90,  112,   89,
 /*   880 */   173,  142,  156,   69,  323,  323,  163,  167,  162,  323,
 /*   890 */   323,  106,  323,  323,  119,  123,  124,  108,   83,   90,
 /*   900 */   112,   89,  173,  142,  156,   69,  323,  323,  163,  167,
 /*   910 */   323,  323,  323,  323,  323,  323,  323,  323,  323,  323,
 /*   920 */   162,  323,  323,  106,  323,  323,  127,  123,  124,  108,
 /*   930 */    83,   90,  112,   89,  173,  142,  156,   69,  323,  323,
 /*   940 */   163,  167,  162,  323,  323,  106,  323,  323,  323,  120,
 /*   950 */   124,  108,   83,   90,  112,   89,  173,  142,  156,   69,
 /*   960 */   323,  323,  163,  167,  162,  323,  323,  106,  323,  323,
 /*   970 */   323,  323,  121,  108,   83,   90,  112,   89,  173,  142,
 /*   980 */   156,   69,  323,  323,  163,  167,   11,   16,   17,   18,
 /*   990 */    19,   20,   21,   22,   23,   24,   25,   26,  323,  162,
 /*  1000 */   323,  323,  106,  323,  323,  323,  323,  323,  104,   83,
 /*  1010 */    90,  112,   89,  173,  142,  156,   69,  323,  323,  163,
 /*  1020 */   167,  162,  323,  323,  106,  323,  323,  154,  155,  323,
 /*  1030 */   323,   81,   90,  112,   89,  173,  142,  156,   69,  323,
 /*  1040 */   162,  163,  167,  106,  323,  323,  323,  323,  323,  323,
 /*  1050 */    82,   90,  112,   89,  173,  142,  156,   69,  323,  323,
 /*  1060 */   163,  167,  162,  323,  323,  106,  323,  323,  323,  323,
 /*  1070 */   323,  323,  323,   84,  112,   89,  173,  142,  156,   69,
 /*  1080 */   323,  323,  163,  167,  162,  323,  323,  106,  323,  323,
 /*  1090 */   323,  323,  323,  323,  323,   86,  112,   89,  173,  142,
 /*  1100 */   156,   69,  323,  162,  163,  167,  106,  323,  323,  323,
 /*  1110 */   323,  323,  323,  323,   88,  112,   89,  173,  142,  156,
 /*  1120 */    69,  323,  323,  163,  167,  323,  162,  323,  323,  106,
 /*  1130 */   323,  323,  323,  323,  323,  323,  323,   91,  112,   89,
 /*  1140 */   173,  142,  156,   69,  323,  323,  163,  167,  323,  162,
 /*  1150 */   323,  323,  106,  323,  323,  323,  323,  323,  323,  323,
 /*  1160 */    92,  112,   89,  173,  142,  156,   69,  323,  323,  163,
 /*  1170 */   167,  162,  323,  323,  106,  323,  323,  323,  323,  323,
 /*  1180 */   323,  323,   93,  112,   89,  173,  142,  156,   69,  323,
 /*  1190 */   162,  163,  167,  106,  323,  323,  323,  323,  323,  323,
 /*  1200 */   323,   94,  112,   89,  173,  142,  156,   69,  323,  323,
 /*  1210 */   163,  167,  323,  162,  323,  323,  106,  323,  323,  323,
 /*  1220 */   323,  323,  323,  323,   95,  112,   89,  173,  142,  156,
 /*  1230 */    69,  323,  323,  163,  167,  323,  162,  323,  323,  106,
 /*  1240 */   323,  323,  323,  323,  323,  323,  323,   96,  112,   89,
 /*  1250 */   173,  142,  156,   69,  323,  323,  163,  167,  162,  323,
 /*  1260 */   323,  106,  323,  323,  323,  323,  323,  323,  323,   97,
 /*  1270 */   112,   89,  173,  142,  156,   69,  323,  162,  163,  167,
 /*  1280 */   106,  323,  323,  323,  323,  323,  323,  323,   98,  112,
 /*  1290 */    89,  173,  142,  156,   69,  323,  323,  163,  167,  323,
 /*  1300 */   162,  323,  323,  106,  323,  323,  323,  323,  323,  323,
 /*  1310 */   323,   99,  112,   89,  173,  142,  156,   69,  323,  323,
 /*  1320 */   163,  167,  323,  162,  323,  323,  106,  323,  323,  323,
 /*  1330 */   323,  323,  323,  323,  100,  112,   89,  173,  142,  156,
 /*  1340 */    69,  323,  323,  163,  167,  162,  323,  323,  106,  323,
 /*  1350 */   323,  323,  323,  323,  323,  323,  323,  105,   89,  173,
 /*  1360 */   142,  156,   69,  323,  162,  163,  167,  106,  323,  323,
 /*  1370 */   323,  323,  323,  323,  323,  323,  109,   89,  173,  142,
 /*  1380 */   156,   69,  323,  323,  163,  167,  323,  162,  323,  323,
 /*  1390 */   106,    6,  323,  323,  166,   79,  323,  323,  323,  111,
 /*  1400 */    89,  173,  142,  156,   69,  323,  323,  163,  167,  323,
 /*  1410 */   162,  323,  323,  106,  323,  323,  323,  323,  323,  162,
 /*  1420 */   323,  323,  106,   85,  173,  142,  156,   69,  323,  323,
 /*  1430 */   163,  167,   87,  173,  142,  156,   69,  323,  323,  163,
 /*  1440 */   167,  162,  323,  323,  106,  323,  323,  323,  168,  169,
 /*  1450 */   170,  171,  172,    2,  323,  141,  142,  156,   69,  323,
 /*  1460 */   162,  163,  167,  106,  323,  323,  323,  323,  323,  323,
 /*  1470 */   162,  323,  323,  106,  143,  142,  156,   69,  323,  162,
 /*  1480 */   163,  167,  106,  323,  144,  142,  156,   69,  323,  162,
 /*  1490 */   163,  167,  106,  145,  142,  156,   69,  323,  162,  163,
 /*  1500 */   167,  106,  323,  146,  142,  156,   69,  323,  162,  163,
 /*  1510 */   167,  106,  147,  142,  156,   69,  323,  162,  163,  167,
 /*  1520 */   106,  323,  148,  142,  156,   69,  323,  162,  163,  167,
 /*  1530 */   106,  149,  142,  156,   69,  323,  162,  163,  167,  106,
 /*  1540 */   323,  150,  142,  156,   69,  323,  162,  163,  167,  106,
 /*  1550 */   151,  142,  156,   69,  323,  162,  163,  167,  106,  323,
 /*  1560 */   152,  142,  156,   69,  323,  162,  163,  167,  106,  153,
 /*  1570 */   142,  156,   69,  323,  162,  163,  167,  106,  323,  161,
 /*  1580 */   142,  156,   69,  323,  323,  163,  167,  323,  165,  142,
 /*  1590 */   156,   69,  323,  323,  163,  167,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    71,   72,   73,   74,   75,   76,   77,   78,   79,   80,
 /*    10 */    81,   82,   83,   84,   85,   86,   87,   88,   89,   90,
 /*    20 */    91,   92,  100,  101,   95,   96,    4,    5,   74,    7,
 /*    30 */     8,    9,   74,   11,   72,   73,   74,   75,   76,   77,
 /*    40 */    78,   79,   80,   81,   82,   83,   84,   85,   86,   87,
 /*    50 */    88,   89,   90,   91,   92,    1,    2,   95,   96,    1,
 /*    60 */     2,    3,    4,    5,    6,    7,    8,    9,   74,   11,
 /*    70 */    48,   49,   45,   46,   47,   53,   54,   55,   56,   57,
 /*    80 */    58,   59,   60,   61,   62,   63,   64,   65,   66,   67,
 /*    90 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   100 */    85,   86,   87,   88,   89,   90,   91,   92,   12,   12,
 /*   110 */    95,   96,   97,   98,   73,   97,   75,   76,   77,   78,
 /*   120 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   88,
 /*   130 */    89,   90,   91,   92,   74,    5,   95,   96,    8,    9,
 /*   140 */    99,   10,   12,   12,   73,   99,   75,   76,   77,   78,
 /*   150 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   88,
 /*   160 */    89,   90,   91,   92,   68,   68,   95,   96,    4,    5,
 /*   170 */     7,    7,    8,    9,   72,   11,   74,   74,   48,   49,
 /*   180 */    75,   93,   94,   53,   54,   55,   56,   57,   58,   59,
 /*   190 */    60,   61,   62,   63,   64,   65,   66,   67,   68,    5,
 /*   200 */    95,   96,    8,    9,    3,   73,   12,   75,   76,   77,
 /*   210 */    78,   79,   80,   81,   82,   83,   84,   85,   86,   87,
 /*   220 */    88,   89,   90,   91,   92,   74,   25,   95,   96,   32,
 /*   230 */    33,   34,   35,   36,   37,   38,   39,   40,   41,   42,
 /*   240 */    43,   44,   48,   49,   30,   31,    5,   53,   54,   55,
 /*   250 */    56,   57,   58,   59,   60,   61,   62,   63,   64,   65,
 /*   260 */    66,   67,   68,    5,   48,   49,    8,    9,   26,   73,
 /*   270 */    12,   75,   76,   77,   78,   79,   80,   81,   82,   83,
 /*   280 */    84,   85,   86,   87,   88,   89,   90,   91,   92,   54,
 /*   290 */    55,   95,   96,    0,    1,    2,    3,    4,    5,   27,
 /*   300 */     7,    8,    9,    6,   11,   28,   48,   49,   67,   12,
 /*   310 */    69,   53,   54,   55,   56,   57,   58,   59,   60,   61,
 /*   320 */    62,   63,   64,   65,   66,   67,   75,   76,   77,   78,
 /*   330 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   88,
 /*   340 */    89,   90,   91,   92,    6,    5,   95,   96,    8,    9,
 /*   350 */    12,   29,    8,  102,   75,   76,   77,   78,   79,   80,
 /*   360 */    81,   82,   83,   84,   85,   86,   87,   88,   89,   90,
 /*   370 */    91,   92,    0,   26,   95,   96,   50,   51,   52,   10,
 /*   380 */   103,   12,  103,  103,   12,  103,  103,  103,   48,   49,
 /*   390 */   103,  103,  103,   53,   54,   55,   56,   57,   58,   59,
 /*   400 */    60,   61,   62,   63,   64,   65,   66,   67,   75,   76,
 /*   410 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   420 */    87,   88,   89,   90,   91,   92,  103,  103,   95,   96,
 /*   430 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   440 */    85,   86,   87,   88,   89,   90,   91,   92,  103,  103,
 /*   450 */    95,   96,  103,  103,  103,  103,  103,  103,  103,  103,
 /*   460 */   103,  103,  103,  103,   75,   76,   77,   78,   79,   80,
 /*   470 */    81,   82,   83,   84,   85,   86,   87,   88,   89,   90,
 /*   480 */    91,   92,    8,  103,   95,   96,   75,   76,   77,   78,
 /*   490 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   88,
 /*   500 */    89,   90,   91,   92,  103,  103,   95,   96,   75,   76,
 /*   510 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   520 */    87,   88,   89,   90,   91,   92,  103,  103,   95,   96,
 /*   530 */   103,  103,  103,  103,  103,  103,   62,  103,   64,  103,
 /*   540 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   550 */    85,   86,   87,   88,   89,   90,   91,   92,  103,  103,
 /*   560 */    95,   96,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   570 */    83,   84,   85,   86,   87,   88,   89,   90,   91,   92,
 /*   580 */   103,  103,   95,   96,   75,   76,   77,   78,   79,   80,
 /*   590 */    81,   82,   83,   84,   85,   86,   87,   88,   89,   90,
 /*   600 */    91,   92,  103,  103,   95,   96,  103,  103,  103,  103,
 /*   610 */   103,  103,  103,  103,  103,  103,   75,   76,   77,   78,
 /*   620 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   88,
 /*   630 */    89,   90,   91,   92,  103,  103,   95,   96,   75,   76,
 /*   640 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   650 */    87,   88,   89,   90,   91,   92,  103,  103,   95,   96,
 /*   660 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   670 */    85,   86,   87,   88,   89,   90,   91,   92,  103,  103,
 /*   680 */    95,   96,  103,  103,  103,  103,  103,  103,  103,  103,
 /*   690 */   103,  103,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   700 */    83,   84,   85,   86,   87,   88,   89,   90,   91,   92,
 /*   710 */   103,  103,   95,   96,   75,   76,   77,   78,   79,   80,
 /*   720 */    81,   82,   83,   84,   85,   86,   87,   88,   89,   90,
 /*   730 */    91,   92,  103,  103,   95,   96,   75,   76,   77,   78,
 /*   740 */    79,   80,   81,   82,   83,   84,   85,   86,   87,   88,
 /*   750 */    89,   90,   91,   92,  103,  103,   95,   96,  103,  103,
 /*   760 */   103,  103,  103,  103,  103,  103,  103,  103,   75,   76,
 /*   770 */    77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
 /*   780 */    87,   88,   89,   90,   91,   92,  103,  103,   95,   96,
 /*   790 */    75,   76,   77,   78,   79,   80,   81,   82,   83,   84,
 /*   800 */    85,   86,   87,   88,   89,   90,   91,   92,  103,  103,
 /*   810 */    95,   96,   75,   76,   77,   78,   79,   80,   81,   82,
 /*   820 */    83,   84,   85,   86,   87,   88,   89,   90,   91,   92,
 /*   830 */   103,  103,   95,   96,  103,  103,  103,  103,  103,  103,
 /*   840 */   103,  103,  103,  103,   75,   76,   77,   78,   79,   80,
 /*   850 */    81,   82,   83,   84,   85,   86,   87,   88,   89,   90,
 /*   860 */    91,   92,  103,  103,   95,   96,   75,  103,  103,   78,
 /*   870 */   103,   80,   81,   82,   83,   84,   85,   86,   87,   88,
 /*   880 */    89,   90,   91,   92,  103,  103,   95,   96,   75,  103,
 /*   890 */   103,   78,  103,  103,   81,   82,   83,   84,   85,   86,
 /*   900 */    87,   88,   89,   90,   91,   92,  103,  103,   95,   96,
 /*   910 */   103,  103,  103,  103,  103,  103,  103,  103,  103,  103,
 /*   920 */    75,  103,  103,   78,  103,  103,   81,   82,   83,   84,
 /*   930 */    85,   86,   87,   88,   89,   90,   91,   92,  103,  103,
 /*   940 */    95,   96,   75,  103,  103,   78,  103,  103,  103,   82,
 /*   950 */    83,   84,   85,   86,   87,   88,   89,   90,   91,   92,
 /*   960 */   103,  103,   95,   96,   75,  103,  103,   78,  103,  103,
 /*   970 */   103,  103,   83,   84,   85,   86,   87,   88,   89,   90,
 /*   980 */    91,   92,  103,  103,   95,   96,   13,   14,   15,   16,
 /*   990 */    17,   18,   19,   20,   21,   22,   23,   24,  103,   75,
 /*  1000 */   103,  103,   78,  103,  103,  103,  103,  103,   84,   85,
 /*  1010 */    86,   87,   88,   89,   90,   91,   92,  103,  103,   95,
 /*  1020 */    96,   75,  103,  103,   78,  103,  103,   54,   55,  103,
 /*  1030 */   103,   85,   86,   87,   88,   89,   90,   91,   92,  103,
 /*  1040 */    75,   95,   96,   78,  103,  103,  103,  103,  103,  103,
 /*  1050 */    85,   86,   87,   88,   89,   90,   91,   92,  103,  103,
 /*  1060 */    95,   96,   75,  103,  103,   78,  103,  103,  103,  103,
 /*  1070 */   103,  103,  103,   86,   87,   88,   89,   90,   91,   92,
 /*  1080 */   103,  103,   95,   96,   75,  103,  103,   78,  103,  103,
 /*  1090 */   103,  103,  103,  103,  103,   86,   87,   88,   89,   90,
 /*  1100 */    91,   92,  103,   75,   95,   96,   78,  103,  103,  103,
 /*  1110 */   103,  103,  103,  103,   86,   87,   88,   89,   90,   91,
 /*  1120 */    92,  103,  103,   95,   96,  103,   75,  103,  103,   78,
 /*  1130 */   103,  103,  103,  103,  103,  103,  103,   86,   87,   88,
 /*  1140 */    89,   90,   91,   92,  103,  103,   95,   96,  103,   75,
 /*  1150 */   103,  103,   78,  103,  103,  103,  103,  103,  103,  103,
 /*  1160 */    86,   87,   88,   89,   90,   91,   92,  103,  103,   95,
 /*  1170 */    96,   75,  103,  103,   78,  103,  103,  103,  103,  103,
 /*  1180 */   103,  103,   86,   87,   88,   89,   90,   91,   92,  103,
 /*  1190 */    75,   95,   96,   78,  103,  103,  103,  103,  103,  103,
 /*  1200 */   103,   86,   87,   88,   89,   90,   91,   92,  103,  103,
 /*  1210 */    95,   96,  103,   75,  103,  103,   78,  103,  103,  103,
 /*  1220 */   103,  103,  103,  103,   86,   87,   88,   89,   90,   91,
 /*  1230 */    92,  103,  103,   95,   96,  103,   75,  103,  103,   78,
 /*  1240 */   103,  103,  103,  103,  103,  103,  103,   86,   87,   88,
 /*  1250 */    89,   90,   91,   92,  103,  103,   95,   96,   75,  103,
 /*  1260 */   103,   78,  103,  103,  103,  103,  103,  103,  103,   86,
 /*  1270 */    87,   88,   89,   90,   91,   92,  103,   75,   95,   96,
 /*  1280 */    78,  103,  103,  103,  103,  103,  103,  103,   86,   87,
 /*  1290 */    88,   89,   90,   91,   92,  103,  103,   95,   96,  103,
 /*  1300 */    75,  103,  103,   78,  103,  103,  103,  103,  103,  103,
 /*  1310 */   103,   86,   87,   88,   89,   90,   91,   92,  103,  103,
 /*  1320 */    95,   96,  103,   75,  103,  103,   78,  103,  103,  103,
 /*  1330 */   103,  103,  103,  103,   86,   87,   88,   89,   90,   91,
 /*  1340 */    92,  103,  103,   95,   96,   75,  103,  103,   78,  103,
 /*  1350 */   103,  103,  103,  103,  103,  103,  103,   87,   88,   89,
 /*  1360 */    90,   91,   92,  103,   75,   95,   96,   78,  103,  103,
 /*  1370 */   103,  103,  103,  103,  103,  103,   87,   88,   89,   90,
 /*  1380 */    91,   92,  103,  103,   95,   96,  103,   75,  103,  103,
 /*  1390 */    78,    5,  103,  103,    8,    9,  103,  103,  103,   87,
 /*  1400 */    88,   89,   90,   91,   92,  103,  103,   95,   96,  103,
 /*  1410 */    75,  103,  103,   78,  103,  103,  103,  103,  103,   75,
 /*  1420 */   103,  103,   78,   88,   89,   90,   91,   92,  103,  103,
 /*  1430 */    95,   96,   88,   89,   90,   91,   92,  103,  103,   95,
 /*  1440 */    96,   75,  103,  103,   78,  103,  103,  103,   62,   63,
 /*  1450 */    64,   65,   66,   67,  103,   89,   90,   91,   92,  103,
 /*  1460 */    75,   95,   96,   78,  103,  103,  103,  103,  103,  103,
 /*  1470 */    75,  103,  103,   78,   89,   90,   91,   92,  103,   75,
 /*  1480 */    95,   96,   78,  103,   89,   90,   91,   92,  103,   75,
 /*  1490 */    95,   96,   78,   89,   90,   91,   92,  103,   75,   95,
 /*  1500 */    96,   78,  103,   89,   90,   91,   92,  103,   75,   95,
 /*  1510 */    96,   78,   89,   90,   91,   92,  103,   75,   95,   96,
 /*  1520 */    78,  103,   89,   90,   91,   92,  103,   75,   95,   96,
 /*  1530 */    78,   89,   90,   91,   92,  103,   75,   95,   96,   78,
 /*  1540 */   103,   89,   90,   91,   92,  103,   75,   95,   96,   78,
 /*  1550 */    89,   90,   91,   92,  103,   75,   95,   96,   78,  103,
 /*  1560 */    89,   90,   91,   92,  103,   75,   95,   96,   78,   89,
 /*  1570 */    90,   91,   92,  103,   75,   95,   96,   78,  103,   89,
 /*  1580 */    90,   91,   92,  103,  103,   95,   96,  103,   89,   90,
 /*  1590 */    91,   92,  103,  103,   95,   96,
};
#define YY_SHIFT_USE_DFLT (-1)
#define YY_SHIFT_MAX 128
static const short yy_shift_ofst[] = {
 /*     0 */    22,   22,  258,  340,  340,  340,  340,  340,  130,  194,
 /*    10 */   340,  340,  340,  340,  340,  340,  340,  340,  340,  340,
 /*    20 */   340,  340,  340,  340,  340,  340,  340,  340,  340,  340,
 /*    30 */   340,  340,  340,  340,  340,  340,  340,  340,  340,  340,
 /*    40 */   340,  340,  340,  340,  340,  340,  340,  340,  340,  340,
 /*    50 */   340,  340,  340,  340,  340,  340,  340,  340,  340,  340,
 /*    60 */   340,  340,  340,  340,  340,  340,  340, 1386,  164,  241,
 /*    70 */   474,  293,   58,  164,  164,  164,  164,  164,   96,   -1,
 /*    80 */   973,  197,  197,  197,   27,  326,   27,  326,   27,  326,
 /*    90 */    27,   27,   27,   27,   27,   27,   27,   27,   27,   27,
 /*   100 */    27,  131,  201,   54,  214,  216,  235,   97,  214,  216,
 /*   110 */   297,  216,  216,  369,  338,   54,  372,  163,  242,  272,
 /*   120 */   277,  322,  272,  277,  322,  347,  344,  272,  163,
};
#define YY_REDUCE_USE_DFLT (-79)
#define YY_REDUCE_MAX 79
static const short yy_reduce_ofst[] = {
 /*     0 */   -71,  -38,   15,   41,   71,  132,  196,  251,  279,  333,
 /*    10 */   355,  389,  411,  433,  465,  487,  509,  541,  563,  585,
 /*    20 */   617,  639,  661,  693,  715,  737,  769,  791,  813,  845,
 /*    30 */   867,  889,  924,  946,  965,  987, 1009, 1028, 1051, 1074,
 /*    40 */  1096, 1115, 1138, 1161, 1183, 1202, 1225, 1248, 1270, 1289,
 /*    50 */  1312, 1335, 1344, 1366, 1385, 1395, 1404, 1414, 1423, 1433,
 /*    60 */  1442, 1452, 1461, 1471, 1480, 1490, 1499,  105,  102,   88,
 /*    70 */   -78,  -46,  -46,  -42,   -6,   60,  103,  151,   18,   46,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   321,  321,  321,  311,  321,  321,  321,  318,  321,  321,
 /*    10 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*    20 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*    30 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*    40 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*    50 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  321,
 /*    60 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  289,
 /*    70 */   321,  321,  321,  321,  321,  321,  321,  321,  321,  311,
 /*    80 */   285,  246,  247,  245,  249,  267,  250,  268,  251,  266,
 /*    90 */   248,  252,  253,  254,  255,  256,  257,  258,  259,  260,
 /*   100 */   261,  321,  232,  234,  244,  263,  285,  321,  243,  264,
 /*   110 */   321,  265,  262,  321,  321,  235,  321,  321,  321,  237,
 /*   120 */   240,  242,  236,  239,  241,  321,  321,  238,  295,  207,
 /*   130 */   208,  211,  206,  209,  213,  214,  215,  218,  219,  220,
 /*   140 */   233,  270,  273,  274,  275,  276,  277,  278,  279,  280,
 /*   150 */   281,  282,  283,  284,  286,  287,  288,  290,  292,  217,
 /*   160 */   315,  271,  291,  293,  294,  272,  295,  296,  297,  298,
 /*   170 */   299,  300,  301,  269,  302,  306,  308,  310,  312,  313,
 /*   180 */   314,  303,  309,  304,  305,  307,  316,  317,  320,  319,
 /*   190 */   221,  222,  223,  224,  225,  226,  227,  228,  229,  230,
 /*   200 */   231,  216,  210,  212,
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
      case 0: /* input ::= query */
      case 1: /* input ::= expression */
      case 2: /* query ::= query_element */
      case 7: /* query_element ::= QSTRING */
      case 8: /* query_element ::= PARENL query PARENR */
      case 13: /* expression ::= assignment_expression */
      case 15: /* assignment_expression ::= conditional_expression */
      case 28: /* conditional_expression ::= logical_or_expression */
      case 30: /* logical_or_expression ::= logical_and_expression */
      case 32: /* logical_and_expression ::= bitwise_or_expression */
      case 35: /* bitwise_or_expression ::= bitwise_xor_expression */
      case 37: /* bitwise_xor_expression ::= bitwise_and_expression */
      case 39: /* bitwise_and_expression ::= equality_expression */
      case 41: /* equality_expression ::= relational_expression */
      case 44: /* relational_expression ::= shift_expression */
      case 58: /* shift_expression ::= additive_expression */
      case 62: /* additive_expression ::= multiplicative_expression */
      case 65: /* multiplicative_expression ::= unary_expression */
      case 69: /* unary_expression ::= postfix_expression */
      case 81: /* postfix_expression ::= lefthand_side_expression */
      case 84: /* lefthand_side_expression ::= call_expression */
      case 85: /* lefthand_side_expression ::= member_expression */
      case 87: /* member_expression ::= primary_expression */
      case 88: /* member_expression ::= member_expression member_expression_part */
      case 89: /* primary_expression ::= object_literal */
      case 90: /* primary_expression ::= PARENL expression PARENR */
      case 91: /* primary_expression ::= IDENTIFIER */
      case 92: /* primary_expression ::= array_literal */
      case 93: /* primary_expression ::= DECIMAL */
      case 94: /* primary_expression ::= HEX_INTEGER */
      case 95: /* primary_expression ::= STRING */
      case 96: /* primary_expression ::= BOOLEAN */
      case 97: /* primary_expression ::= NULL */
      case 98: /* array_literal ::= BRACKETL elision BRACKETR */
      case 99: /* array_literal ::= BRACKETL element_list elision BRACKETR */
      case 100: /* array_literal ::= BRACKETL element_list BRACKETR */
      case 101: /* elision ::= COMMA */
      case 102: /* elision ::= elision COMMA */
      case 103: /* element_list ::= assignment_expression */
      case 104: /* element_list ::= elision assignment_expression */
      case 105: /* element_list ::= element_list elision assignment_expression */
      case 106: /* object_literal ::= BRACEL property_name_and_value_list BRACER */
      case 107: /* property_name_and_value_list ::= */
      case 108: /* property_name_and_value_list ::= property_name_and_value_list COMMA property_name_and_value */
      case 109: /* property_name_and_value ::= property_name COLON assignment_expression */
      case 110: /* property_name ::= IDENTIFIER|STRING|DECIMAL */
      case 111: /* member_expression_part ::= BRACKETL expression BRACKETR */
      case 112: /* member_expression_part ::= DOT IDENTIFIER */
#line 25 "ecmascript.y"
{
}
#line 1250 "ecmascript.c"
        break;
      case 3: /* query ::= query query_element */
#line 29 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, grn_int32_value_at(&efsi->op_stack, -1), 2);
}
#line 1257 "ecmascript.c"
        break;
      case 4: /* query ::= query LOGICAL_AND query_element */
      case 33: /* logical_and_expression ::= logical_and_expression LOGICAL_AND bitwise_or_expression */
#line 32 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND, 2);
}
#line 1265 "ecmascript.c"
        break;
      case 5: /* query ::= query LOGICAL_BUT query_element */
      case 34: /* logical_and_expression ::= logical_and_expression LOGICAL_BUT bitwise_or_expression */
#line 35 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BUT, 2);
}
#line 1273 "ecmascript.c"
        break;
      case 6: /* query ::= query LOGICAL_OR query_element */
      case 31: /* logical_or_expression ::= logical_or_expression LOGICAL_OR logical_and_expression */
#line 38 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_OR, 2);
}
#line 1281 "ecmascript.c"
        break;
      case 9: /* query_element ::= RELATIVE_OP query_element */
#line 45 "ecmascript.y"
{
  int mode;
  GRN_UINT32_POP(&efsi->mode_stack, mode);
}
#line 1289 "ecmascript.c"
        break;
      case 10: /* query_element ::= IDENTIFIER RELATIVE_OP query_element */
#line 49 "ecmascript.y"
{
  int mode;
  grn_obj *c;
  GRN_PTR_POP(&efsi->column_stack, c);
  GRN_UINT32_POP(&efsi->mode_stack, mode);
}
#line 1299 "ecmascript.c"
        break;
      case 11: /* query_element ::= BRACEL expression BRACER */
      case 12: /* query_element ::= EVAL primary_expression */
#line 55 "ecmascript.y"
{
  efsi->flags = efsi->default_flags;
}
#line 1307 "ecmascript.c"
        break;
      case 14: /* expression ::= expression COMMA assignment_expression */
#line 63 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_COMMA, 2);
}
#line 1314 "ecmascript.c"
        break;
      case 16: /* assignment_expression ::= lefthand_side_expression ASSIGN assignment_expression */
#line 68 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_ASSIGN, 2);
}
#line 1321 "ecmascript.c"
        break;
      case 17: /* assignment_expression ::= lefthand_side_expression STAR_ASSIGN assignment_expression */
#line 71 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_STAR_ASSIGN, 2);
}
#line 1328 "ecmascript.c"
        break;
      case 18: /* assignment_expression ::= lefthand_side_expression SLASH_ASSIGN assignment_expression */
#line 74 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SLASH_ASSIGN, 2);
}
#line 1335 "ecmascript.c"
        break;
      case 19: /* assignment_expression ::= lefthand_side_expression MOD_ASSIGN assignment_expression */
#line 77 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MOD_ASSIGN, 2);
}
#line 1342 "ecmascript.c"
        break;
      case 20: /* assignment_expression ::= lefthand_side_expression PLUS_ASSIGN assignment_expression */
#line 80 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PLUS_ASSIGN, 2);
}
#line 1349 "ecmascript.c"
        break;
      case 21: /* assignment_expression ::= lefthand_side_expression MINUS_ASSIGN assignment_expression */
#line 83 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MINUS_ASSIGN, 2);
}
#line 1356 "ecmascript.c"
        break;
      case 22: /* assignment_expression ::= lefthand_side_expression SHIFTL_ASSIGN assignment_expression */
#line 86 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTL_ASSIGN, 2);
}
#line 1363 "ecmascript.c"
        break;
      case 23: /* assignment_expression ::= lefthand_side_expression SHIFTR_ASSIGN assignment_expression */
#line 89 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTR_ASSIGN, 2);
}
#line 1370 "ecmascript.c"
        break;
      case 24: /* assignment_expression ::= lefthand_side_expression SHIFTRR_ASSIGN assignment_expression */
#line 92 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTRR_ASSIGN, 2);
}
#line 1377 "ecmascript.c"
        break;
      case 25: /* assignment_expression ::= lefthand_side_expression AND_ASSIGN assignment_expression */
#line 95 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND_ASSIGN, 2);
}
#line 1384 "ecmascript.c"
        break;
      case 26: /* assignment_expression ::= lefthand_side_expression XOR_ASSIGN assignment_expression */
#line 98 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_XOR_ASSIGN, 2);
}
#line 1391 "ecmascript.c"
        break;
      case 27: /* assignment_expression ::= lefthand_side_expression OR_ASSIGN assignment_expression */
#line 101 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_OR_ASSIGN, 2);
}
#line 1398 "ecmascript.c"
        break;
      case 29: /* conditional_expression ::= logical_or_expression QUESTION assignment_expression COLON assignment_expression */
#line 106 "ecmascript.y"
{
  grn_expr *e = (grn_expr *)efsi->e;
  e->codes[yymsp[-3].minor.yy0].nargs = yymsp[-1].minor.yy0 - yymsp[-3].minor.yy0;
  e->codes[yymsp[-1].minor.yy0].nargs = e->codes_curr - yymsp[-1].minor.yy0 - 1;
}
#line 1407 "ecmascript.c"
        break;
      case 36: /* bitwise_or_expression ::= bitwise_or_expression BITWISE_OR bitwise_xor_expression */
#line 126 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_OR, 2);
}
#line 1414 "ecmascript.c"
        break;
      case 38: /* bitwise_xor_expression ::= bitwise_xor_expression BITWISE_XOR bitwise_and_expression */
#line 131 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_XOR, 2);
}
#line 1421 "ecmascript.c"
        break;
      case 40: /* bitwise_and_expression ::= bitwise_and_expression BITWISE_AND equality_expression */
#line 136 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_AND, 2);
}
#line 1428 "ecmascript.c"
        break;
      case 42: /* equality_expression ::= equality_expression EQUAL relational_expression */
#line 141 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_EQUAL, 2);
}
#line 1435 "ecmascript.c"
        break;
      case 43: /* equality_expression ::= equality_expression NOT_EQUAL relational_expression */
#line 144 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NOT_EQUAL, 2);
}
#line 1442 "ecmascript.c"
        break;
      case 45: /* relational_expression ::= relational_expression LESS shift_expression */
#line 149 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_LESS, 2);
}
#line 1449 "ecmascript.c"
        break;
      case 46: /* relational_expression ::= relational_expression GREATER shift_expression */
#line 152 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_GREATER, 2);
}
#line 1456 "ecmascript.c"
        break;
      case 47: /* relational_expression ::= relational_expression LESS_EQUAL shift_expression */
#line 155 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_LESS_EQUAL, 2);
}
#line 1463 "ecmascript.c"
        break;
      case 48: /* relational_expression ::= relational_expression GREATER_EQUAL shift_expression */
#line 158 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_GREATER_EQUAL, 2);
}
#line 1470 "ecmascript.c"
        break;
      case 49: /* relational_expression ::= relational_expression IN shift_expression */
#line 161 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_IN, 2);
}
#line 1477 "ecmascript.c"
        break;
      case 50: /* relational_expression ::= relational_expression MATCH shift_expression */
#line 164 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MATCH, 2);
}
#line 1484 "ecmascript.c"
        break;
      case 51: /* relational_expression ::= relational_expression NEAR shift_expression */
#line 167 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NEAR, 2);
}
#line 1491 "ecmascript.c"
        break;
      case 52: /* relational_expression ::= relational_expression NEAR2 shift_expression */
#line 170 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NEAR2, 2);
}
#line 1498 "ecmascript.c"
        break;
      case 53: /* relational_expression ::= relational_expression SIMILAR shift_expression */
#line 173 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SIMILAR, 2);
}
#line 1505 "ecmascript.c"
        break;
      case 54: /* relational_expression ::= relational_expression TERM_EXTRACT shift_expression */
#line 176 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_TERM_EXTRACT, 2);
}
#line 1512 "ecmascript.c"
        break;
      case 55: /* relational_expression ::= relational_expression LCP shift_expression */
#line 179 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_LCP, 2);
}
#line 1519 "ecmascript.c"
        break;
      case 56: /* relational_expression ::= relational_expression PREFIX shift_expression */
#line 182 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PREFIX, 2);
}
#line 1526 "ecmascript.c"
        break;
      case 57: /* relational_expression ::= relational_expression SUFFIX shift_expression */
#line 185 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SUFFIX, 2);
}
#line 1533 "ecmascript.c"
        break;
      case 59: /* shift_expression ::= shift_expression SHIFTL additive_expression */
#line 190 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTL, 2);
}
#line 1540 "ecmascript.c"
        break;
      case 60: /* shift_expression ::= shift_expression SHIFTR additive_expression */
#line 193 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTR, 2);
}
#line 1547 "ecmascript.c"
        break;
      case 61: /* shift_expression ::= shift_expression SHIFTRR additive_expression */
#line 196 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTRR, 2);
}
#line 1554 "ecmascript.c"
        break;
      case 63: /* additive_expression ::= additive_expression PLUS multiplicative_expression */
#line 201 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PLUS, 2);
}
#line 1561 "ecmascript.c"
        break;
      case 64: /* additive_expression ::= additive_expression MINUS multiplicative_expression */
#line 204 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MINUS, 2);
}
#line 1568 "ecmascript.c"
        break;
      case 66: /* multiplicative_expression ::= multiplicative_expression STAR unary_expression */
#line 209 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_STAR, 2);
}
#line 1575 "ecmascript.c"
        break;
      case 67: /* multiplicative_expression ::= multiplicative_expression SLASH unary_expression */
#line 212 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SLASH, 2);
}
#line 1582 "ecmascript.c"
        break;
      case 68: /* multiplicative_expression ::= multiplicative_expression MOD unary_expression */
#line 215 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MOD, 2);
}
#line 1589 "ecmascript.c"
        break;
      case 70: /* unary_expression ::= DELETE unary_expression */
#line 220 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_DELETE, 1);
}
#line 1596 "ecmascript.c"
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
#line 1617 "ecmascript.c"
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
#line 1638 "ecmascript.c"
        break;
      case 73: /* unary_expression ::= PLUS unary_expression */
#line 257 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PLUS, 1);
}
#line 1645 "ecmascript.c"
        break;
      case 74: /* unary_expression ::= MINUS unary_expression */
#line 260 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MINUS, 1);
}
#line 1652 "ecmascript.c"
        break;
      case 75: /* unary_expression ::= NOT unary_expression */
#line 263 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NOT, 1);
}
#line 1659 "ecmascript.c"
        break;
      case 76: /* unary_expression ::= BITWISE_NOT unary_expression */
#line 266 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_NOT, 1);
}
#line 1666 "ecmascript.c"
        break;
      case 77: /* unary_expression ::= ADJUST unary_expression */
#line 269 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_ADJUST, 1);
}
#line 1673 "ecmascript.c"
        break;
      case 78: /* unary_expression ::= EXACT unary_expression */
#line 272 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_EXACT, 1);
}
#line 1680 "ecmascript.c"
        break;
      case 79: /* unary_expression ::= PARTIAL unary_expression */
#line 275 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PARTIAL, 1);
}
#line 1687 "ecmascript.c"
        break;
      case 80: /* unary_expression ::= UNSPLIT unary_expression */
#line 278 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_UNSPLIT, 1);
}
#line 1694 "ecmascript.c"
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
#line 1715 "ecmascript.c"
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
#line 1736 "ecmascript.c"
        break;
      case 86: /* call_expression ::= member_expression arguments */
#line 321 "ecmascript.y"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_CALL, yymsp[0].minor.yy0);
}
#line 1743 "ecmascript.c"
        break;
      case 113: /* arguments ::= PARENL argument_list PARENR */
#line 360 "ecmascript.y"
{ yygotominor.yy0 = yymsp[-1].minor.yy0; }
#line 1748 "ecmascript.c"
        break;
      case 114: /* argument_list ::= */
#line 361 "ecmascript.y"
{ yygotominor.yy0 = 0; }
#line 1753 "ecmascript.c"
        break;
      case 115: /* argument_list ::= assignment_expression */
#line 362 "ecmascript.y"
{ yygotominor.yy0 = 1; }
#line 1758 "ecmascript.c"
        break;
      case 116: /* argument_list ::= argument_list COMMA assignment_expression */
#line 363 "ecmascript.y"
{ yygotominor.yy0 = yymsp[-2].minor.yy0 + 1; }
#line 1763 "ecmascript.c"
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
#line 1836 "ecmascript.c"
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
