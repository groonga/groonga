/*
** 2000-05-29
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Driver template for the LEMON parser generator.
**
** The "lemon" program processes an LALR(1) input grammar file, then uses
** this template to construct a parser.  The "lemon" program inserts text
** at each "%%" line.  Also, any "P-a-r-s-e" identifer prefix (without the
** interstitial "-" characters) contained in this template is changed into
** the value of the %name directive from the grammar.  Otherwise, the content
** of this template is copied straight through into the generate parser
** source file.
**
** The following is the concatenation of all %include directives from the
** input grammar file:
*/
#include <stdio.h>
#include <assert.h>
/************ Begin %include sections from the grammar ************************/
#line 4 "../../groonga/lib/grn_ecmascript.lemon"

#ifdef assert
#  undef assert
#endif
#define assert GRN_ASSERT
#line 35 "../../groonga/lib/grn_ecmascript.c"
/**************** End of %include directives **********************************/
/* These constants specify the various numeric values for terminal symbols
** in a format understandable to "makeheaders".  This section is blank unless
** "lemon" is run with the "-m" command-line option.
***************** Begin makeheaders token definitions *************************/
/**************** End makeheaders token definitions ***************************/

/* The next sections is a series of control #defines.
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used to store the integer codes
**                       that represent terminal and non-terminal symbols.
**                       "unsigned char" is used if there are fewer than
**                       256 symbols.  Larger types otherwise.
**    YYNOCODE           is a number of type YYCODETYPE that is not used for
**                       any terminal or nonterminal symbol.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       (also known as: "terminal symbols") have fall-back
**                       values which should be used if the original symbol
**                       would not parse.  This permits keywords to sometimes
**                       be used as identifiers, for example.
**    YYACTIONTYPE       is the data type used for "action codes" - numbers
**                       that indicate what to do in response to the next
**                       token.
**    grn_expr_parserTOKENTYPE     is the data type used for minor type for terminal
**                       symbols.  Background: A "minor type" is a semantic
**                       value associated with a terminal or non-terminal
**                       symbols.  For example, for an "ID" terminal symbol,
**                       the minor type might be the name of the identifier.
**                       Each non-terminal can have a different minor type.
**                       Terminal symbols all have the same minor type, though.
**                       This macros defines the minor type for terminal 
**                       symbols.
**    YYMINORTYPE        is the data type used for all minor types.
**                       This is typically a union of many types, one of
**                       which is grn_expr_parserTOKENTYPE.  The entry in the union
**                       for terminal symbols is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    grn_expr_parserARG_SDECL     A static variable declaration for the %extra_argument
**    grn_expr_parserARG_PDECL     A parameter declaration for the %extra_argument
**    grn_expr_parserARG_PARAM     Code to pass %extra_argument as a subroutine parameter
**    grn_expr_parserARG_STORE     Code to store %extra_argument into yypParser
**    grn_expr_parserARG_FETCH     Code to extract %extra_argument from yypParser
**    grn_expr_parserCTX_*         As grn_expr_parserARG_ except for %extra_context
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYNTOKEN           Number of terminal symbols
**    YY_MAX_SHIFT       Maximum value for shift actions
**    YY_MIN_SHIFTREDUCE Minimum value for shift-reduce actions
**    YY_MAX_SHIFTREDUCE Maximum value for shift-reduce actions
**    YY_ERROR_ACTION    The yy_action[] code for syntax error
**    YY_ACCEPT_ACTION   The yy_action[] code for accept
**    YY_NO_ACTION       The yy_action[] code for no-op
**    YY_MIN_REDUCE      Minimum value for reduce actions
**    YY_MAX_REDUCE      Maximum value for reduce actions
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/************* Begin control #defines *****************************************/
#define YYCODETYPE unsigned char
#define YYNOCODE 113
#define YYACTIONTYPE unsigned short int
#define grn_expr_parserTOKENTYPE  int 
typedef union {
  int yyinit;
  grn_expr_parserTOKENTYPE yy0;
  void * yy165;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define grn_expr_parserARG_SDECL  efs_info *efsi ;
#define grn_expr_parserARG_PDECL , efs_info *efsi 
#define grn_expr_parserARG_PARAM ,efsi 
#define grn_expr_parserARG_FETCH  efs_info *efsi =yypParser->efsi ;
#define grn_expr_parserARG_STORE yypParser->efsi =efsi ;
#define grn_expr_parserCTX_SDECL
#define grn_expr_parserCTX_PDECL
#define grn_expr_parserCTX_PARAM
#define grn_expr_parserCTX_FETCH
#define grn_expr_parserCTX_STORE
#define YYNSTATE             146
#define YYNRULE              133
#define YYNTOKEN             76
#define YY_MAX_SHIFT         145
#define YY_MIN_SHIFTREDUCE   228
#define YY_MAX_SHIFTREDUCE   360
#define YY_ERROR_ACTION      361
#define YY_ACCEPT_ACTION     362
#define YY_NO_ACTION         363
#define YY_MIN_REDUCE        364
#define YY_MAX_REDUCE        496
/************* End control #defines *******************************************/
#define YY_NLOOKAHEAD ((int)(sizeof(yy_lookahead)/sizeof(yy_lookahead[0])))

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
**   0 <= N <= YY_MAX_SHIFT             Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   N between YY_MIN_SHIFTREDUCE       Shift to an arbitrary state then
**     and YY_MAX_SHIFTREDUCE           reduce by rule N-YY_MIN_SHIFTREDUCE.
**
**   N == YY_ERROR_ACTION               A syntax error has occurred.
**
**   N == YY_ACCEPT_ACTION              The parser accepts its input.
**
**   N == YY_NO_ACTION                  No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
**   N between YY_MIN_REDUCE            Reduce by rule N-YY_MIN_REDUCE
**     and YY_MAX_REDUCE
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as either:
**
**    (A)   N = yy_action[ yy_shift_ofst[S] + X ]
**    (B)   N = yy_default[S]
**
** The (A) formula is preferred.  The B formula is used instead if
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X.
**
** The formulas above are for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array.
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
**
*********** Begin parsing tables **********************************************/
#define YY_ACTTAB_COUNT (1663)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   364,    3,   72,  115,  115,  137,  368,  323,    2,  367,
 /*    10 */    54,   83,  130,    1,  371,   71,  362,   79,  112,   82,
 /*    20 */   373,   79,   75,  112,  112,   90,  127,  126,  140,  139,
 /*    30 */   138,  121,   87,  103,  117,  104,  104,  104,   90,   75,
 /*    40 */   373,  373,   75,   75,  323,   74,  132,   84,   83,  145,
 /*    50 */     9,  370,   71,  369,   66,   65,   53,   52,   51,   69,
 /*    60 */    68,   67,   64,   63,   61,   60,   59,  348,  349,  350,
 /*    70 */   351,  352,    6,  128,   70,   58,   57,   75,  128,  128,
 /*    80 */    90,  127,  126,  140,  139,  138,  121,   87,  103,  117,
 /*    90 */   104,  104,  104,   90,   75,   78,  456,   75,   75,   78,
 /*   100 */   115,  115,  137,  432,  479,  323,    2,  110,   54,   83,
 /*   110 */   130,    1,    4,   71,   78,  119,  491,  137,   78,   75,
 /*   120 */   119,  119,   90,  127,  126,  140,  139,  138,  121,   87,
 /*   130 */   103,  117,  104,  104,  104,   90,   75,  110,  134,   75,
 /*   140 */    75,    7,  300,   62,   77,  346,   73,   30,   29,  451,
 /*   150 */   134,  356,   66,   65,   56,   55,  366,   69,   68,   67,
 /*   160 */    64,   63,   61,   60,   59,  348,  349,  350,  351,  352,
 /*   170 */     6,   50,   49,   48,   47,   46,   45,   44,   43,   42,
 /*   180 */    41,   40,   39,   38,   37,   36,  365,   66,   65,  312,
 /*   190 */    35,   34,   69,   68,   67,   64,   63,   61,   60,   59,
 /*   200 */   348,  349,  350,  351,  352,    6,  111,  457,  313,   75,
 /*   210 */   450,  450,   90,  127,  126,  140,  139,  138,  121,   87,
 /*   220 */   103,  117,  104,  104,  104,   90,   75,  304,  345,   75,
 /*   230 */    75,  492,   23,   27,   21,   27,  445,   75,  450,  450,
 /*   240 */    90,  127,  126,  140,  139,  138,  121,   87,  103,  117,
 /*   250 */   104,  104,  104,   90,   75,  294,  295,   75,   75,  317,
 /*   260 */   236,   76,   27,  131,  447,  316,  133,   31,  357,   22,
 /*   270 */    33,   75,  442,  442,   90,  127,  126,  140,  139,  138,
 /*   280 */   121,   87,  103,  117,  104,  104,  104,   90,   75,  297,
 /*   290 */   303,   75,   75,   32,   24,  363,  363,  114,   75,  435,
 /*   300 */   435,   90,  127,  126,  140,  139,  138,  121,   87,  103,
 /*   310 */   117,  104,  104,  104,   90,   75,   28,  119,   75,   75,
 /*   320 */   116,   75,  119,  119,   90,  127,  126,  140,  139,  138,
 /*   330 */   121,   87,  103,  117,  104,  104,  104,   90,   75,  363,
 /*   340 */    25,   75,   75,  363,  122,  363,  363,  363,   75,  122,
 /*   350 */   122,   90,  127,  126,  140,  139,  138,  121,   87,  103,
 /*   360 */   117,  104,  104,  104,   90,   75,  128,  363,   75,   75,
 /*   370 */    75,  128,  128,   90,  127,  126,  140,  139,  138,  121,
 /*   380 */    87,  103,  117,  104,  104,  104,   90,   75,  363,  363,
 /*   390 */    75,   75,    7,  363,   62,  363,  346,   73,   75,  386,
 /*   400 */   386,   90,  127,  126,  140,  139,  138,  121,   87,  103,
 /*   410 */   117,  104,  104,  104,   90,   75,  363,  363,   75,   75,
 /*   420 */    75,  385,  385,   90,  127,  126,  140,  139,  138,  121,
 /*   430 */    87,  103,  117,  104,  104,  104,   90,   75,   66,   65,
 /*   440 */    75,   75,  455,   69,   68,   67,   64,   63,   61,   60,
 /*   450 */    59,  348,  349,  129,  351,  352,    6,    7,   27,   62,
 /*   460 */   363,  346,   73,   75,  384,  384,   90,  127,  126,  140,
 /*   470 */   139,  138,  121,   87,  103,  117,  104,  104,  104,   90,
 /*   480 */    75,  363,  363,   75,   75,   75,  383,  383,   90,  127,
 /*   490 */   126,  140,  139,  138,  121,   87,  103,  117,  104,  104,
 /*   500 */   104,   90,   75,   66,   65,   75,   75,  363,   69,   68,
 /*   510 */    67,   64,   63,   61,   60,   59,  348,  349,  350,  351,
 /*   520 */   352,    6,   75,  382,  382,   90,  127,  126,  140,  139,
 /*   530 */   138,  121,   87,  103,  117,  104,  104,  104,   90,   75,
 /*   540 */   363,  363,   75,   75,  363,   75,  381,  381,   90,  127,
 /*   550 */   126,  140,  139,  138,  121,   87,  103,  117,  104,  104,
 /*   560 */   104,   90,   75,  363,  363,   75,   75,   75,  380,  380,
 /*   570 */    90,  127,  126,  140,  139,  138,  121,   87,  103,  117,
 /*   580 */   104,  104,  104,   90,   75,  363,  363,   75,   75,   75,
 /*   590 */   379,  379,   90,  127,  126,  140,  139,  138,  121,   87,
 /*   600 */   103,  117,  104,  104,  104,   90,   75,  363,  363,   75,
 /*   610 */    75,   75,  378,  378,   90,  127,  126,  140,  139,  138,
 /*   620 */   121,   87,  103,  117,  104,  104,  104,   90,   75,  363,
 /*   630 */   363,   75,   75,   75,  377,  377,   90,  127,  126,  140,
 /*   640 */   139,  138,  121,   87,  103,  117,  104,  104,  104,   90,
 /*   650 */    75,  363,  363,   75,   75,   75,  376,  376,   90,  127,
 /*   660 */   126,  140,  139,  138,  121,   87,  103,  117,  104,  104,
 /*   670 */   104,   90,   75,  363,  363,   75,   75,   75,  443,  443,
 /*   680 */    90,  127,  126,  140,  139,  138,  121,   87,  103,  117,
 /*   690 */   104,  104,  104,   90,   75,  363,  363,   75,   75,   75,
 /*   700 */   438,  438,   90,  127,  126,  140,  139,  138,  121,   87,
 /*   710 */   103,  117,  104,  104,  104,   90,   75,  363,  363,   75,
 /*   720 */    75,   75,  489,  489,   90,  127,  126,  140,  139,  138,
 /*   730 */   121,   87,  103,  117,  104,  104,  104,   90,   75,  363,
 /*   740 */   363,   75,   75,   75,  387,  387,   90,  127,  126,  140,
 /*   750 */   139,  138,  121,   87,  103,  117,  104,  104,  104,   90,
 /*   760 */    75,  363,  363,   75,   75,   75,  144,  144,   90,  127,
 /*   770 */   126,  140,  139,  138,  121,   87,  103,  117,  104,  104,
 /*   780 */   104,   90,   75,  363,  363,   75,   75,   75,  375,  375,
 /*   790 */    90,  127,  126,  140,  139,  138,  121,   87,  103,  117,
 /*   800 */   104,  104,  104,   90,   75,  363,  363,   75,   75,   75,
 /*   810 */   374,  374,   90,  127,  126,  140,  139,  138,  121,   87,
 /*   820 */   103,  117,  104,  104,  104,   90,   75,  363,   75,   75,
 /*   830 */    75,  123,  363,  113,  140,  139,  138,  121,   87,  103,
 /*   840 */   117,  104,  104,  104,  123,   75,  363,   75,   75,   75,
 /*   850 */   123,  363,  363,  135,  139,  138,  121,   87,  103,  117,
 /*   860 */   104,  104,  104,  123,   75,  363,   75,   75,   75,  123,
 /*   870 */   363,  363,  143,  139,  138,  121,   87,  103,  117,  104,
 /*   880 */   104,  104,  123,   75,    5,   75,   75,   75,  123,  363,
 /*   890 */   363,  363,  142,  138,  121,   87,  103,  117,  104,  104,
 /*   900 */   104,  123,   75,  363,   75,   75,   75,  123,  363,  363,
 /*   910 */   363,  363,  141,  121,   87,  103,  117,  104,  104,  104,
 /*   920 */   123,   75,  363,  363,   75,   75,  363,  363,  363,  363,
 /*   930 */    26,   20,   19,   18,   17,   16,   15,   14,   13,   12,
 /*   940 */    11,   10,   75,  363,  363,  123,  363,  363,    8,  363,
 /*   950 */   136,  125,   87,  103,  117,  104,  104,  104,  123,   75,
 /*   960 */   363,  363,   75,   75,  363,   75,  363,  363,  123,  363,
 /*   970 */   363,  363,  363,  294,  295,   88,  103,  117,  104,  104,
 /*   980 */   104,  123,   75,  363,   75,   75,   75,  123,  363,  363,
 /*   990 */   363,  363,  363,  363,   89,  103,  117,  104,  104,  104,
 /*  1000 */   123,   75,  363,  363,   75,   75,  363,  363,   86,   85,
 /*  1010 */    81,   80,  323,   74,  324,   84,   83,  145,    9,  454,
 /*  1020 */    71,  363,   86,   85,   81,   80,  323,   74,  363,   84,
 /*  1030 */    83,  145,    9,  363,   71,   75,  363,  363,  123,  363,
 /*  1040 */   363,  363,  363,  363,  363,  363,   91,  117,  104,  104,
 /*  1050 */   104,  123,   75,  363,  363,   75,   75,   75,  363,  363,
 /*  1060 */   123,  363,  363,  363,  363,  363,  363,  363,   92,  117,
 /*  1070 */   104,  104,  104,  123,   75,  363,  363,   75,   75,   75,
 /*  1080 */   363,  363,  123,  363,  363,  363,  363,  363,  363,  363,
 /*  1090 */    93,  117,  104,  104,  104,  123,   75,  363,   75,   75,
 /*  1100 */    75,  123,  363,  363,  363,  363,  363,  363,  363,   94,
 /*  1110 */   117,  104,  104,  104,  123,   75,  363,  363,   75,   75,
 /*  1120 */   363,  363,   75,  363,  363,  123,  363,    7,  363,  363,
 /*  1130 */   363,  346,   73,   95,  117,  104,  104,  104,  123,   75,
 /*  1140 */   363,   75,   75,   75,  123,  363,  363,  363,  363,  363,
 /*  1150 */   363,  363,   96,  117,  104,  104,  104,  123,   75,  363,
 /*  1160 */   363,   75,   75,   75,  363,  363,  123,  363,  363,  363,
 /*  1170 */   363,  363,  363,  363,   97,  117,  104,  104,  104,  123,
 /*  1180 */    75,  363,   75,   75,   75,  123,  348,  349,  350,  351,
 /*  1190 */   352,    6,  363,   98,  117,  104,  104,  104,  123,   75,
 /*  1200 */   363,  363,   75,   75,  363,  363,   75,  363,  363,  123,
 /*  1210 */   363,  363,  363,  363,  363,  363,  363,   99,  117,  104,
 /*  1220 */   104,  104,  123,   75,  363,   75,   75,   75,  123,  363,
 /*  1230 */   363,  363,  363,  363,  363,  363,  100,  117,  104,  104,
 /*  1240 */   104,  123,   75,  363,  363,   75,   75,   75,  363,  363,
 /*  1250 */   123,  363,  363,  363,  363,  363,  363,  363,  101,  117,
 /*  1260 */   104,  104,  104,  123,   75,  363,   75,   75,   75,  123,
 /*  1270 */   363,  363,  363,  363,  363,  363,  363,  102,  117,  104,
 /*  1280 */   104,  104,  123,   75,  363,  363,   75,   75,  363,  363,
 /*  1290 */    75,  363,  363,  123,  363,  363,  363,  363,  363,  363,
 /*  1300 */   363,  105,  117,  104,  104,  104,  123,   75,  363,   75,
 /*  1310 */    75,   75,  123,  363,  363,  363,  363,  363,  363,  363,
 /*  1320 */   107,  117,  104,  104,  104,  123,   75,  363,  363,   75,
 /*  1330 */    75,   75,  363,  363,  123,  363,  363,  363,  363,  363,
 /*  1340 */   363,  363,  109,  117,  104,  104,  104,  123,   75,  363,
 /*  1350 */    75,   75,   75,  123,  363,  363,  363,  363,  363,  363,
 /*  1360 */   363,  363,  118,  104,  104,  104,  123,   75,  363,  363,
 /*  1370 */    75,   75,  363,  363,   75,  363,  363,  123,  363,  363,
 /*  1380 */   363,  363,  363,  363,  363,  363,  120,  104,  104,  104,
 /*  1390 */   123,   75,  363,   75,   75,   75,  123,  363,  363,  363,
 /*  1400 */   363,  363,  363,  363,  363,  124,  104,  104,  104,  123,
 /*  1410 */    75,  363,  363,   75,   75,  363,  363,  369,   75,  363,
 /*  1420 */   363,  123,  363,  363,  363,  363,  363,   75,  363,  363,
 /*  1430 */   123,  363,  426,  426,  123,   75,  363,  363,   75,   75,
 /*  1440 */   106,  106,  106,  123,   75,  363,   75,   75,   75,  123,
 /*  1450 */   363,  363,  363,  363,   75,  363,  363,  123,  363,  108,
 /*  1460 */   108,  108,  123,   75,  363,  363,   75,   75,  418,  418,
 /*  1470 */   123,   75,  363,   75,   75,   75,  123,  363,  363,  363,
 /*  1480 */   363,  363,  363,  363,  363,  363,  363,  417,  417,  123,
 /*  1490 */    75,  363,   75,   75,   75,  123,  363,  363,  363,  363,
 /*  1500 */   363,   75,  363,  363,  123,  363,  429,  429,  123,   75,
 /*  1510 */   363,   75,   75,   75,  123,  428,  428,  123,   75,  363,
 /*  1520 */   363,   75,   75,  363,  363,  427,  427,  123,   75,  363,
 /*  1530 */    75,   75,   75,  123,  363,  363,  363,  363,   75,  363,
 /*  1540 */   363,  123,  363,  363,  426,  426,  123,   75,  363,  363,
 /*  1550 */    75,   75,  425,  425,  123,   75,  363,   75,   75,   75,
 /*  1560 */   123,  363,  363,  363,  363,  363,  363,  363,  363,  363,
 /*  1570 */   363,  424,  424,  123,   75,  363,   75,   75,   75,  123,
 /*  1580 */   363,  363,  363,  363,  363,   75,  363,  363,  123,  363,
 /*  1590 */   423,  423,  123,   75,  363,   75,   75,   75,  123,  422,
 /*  1600 */   422,  123,   75,  363,  363,   75,   75,  363,  363,  421,
 /*  1610 */   421,  123,   75,  363,   75,   75,   75,  123,  363,  363,
 /*  1620 */   363,  363,   75,  363,  363,  123,  363,  363,  420,  420,
 /*  1630 */   123,   75,  363,  363,   75,   75,  419,  419,  123,   75,
 /*  1640 */   363,   75,   75,   75,  123,  363,  363,  363,  363,  363,
 /*  1650 */   363,  363,  363,  363,  363,  416,  416,  123,   75,  363,
 /*  1660 */   363,   75,   75,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    82,    1,    2,  106,  107,  108,   82,    7,    8,   82,
 /*    10 */    10,   11,   12,   13,   82,   15,   77,   78,   79,   11,
 /*    20 */    83,   82,   83,   84,   85,   86,   87,   88,   89,   90,
 /*    30 */    91,   92,   93,   94,   95,   96,   97,   98,   99,  100,
 /*    40 */   103,  104,  103,  104,    7,    8,   12,   10,   11,   12,
 /*    50 */    13,   82,   15,   82,   54,   55,   51,   52,   53,   59,
 /*    60 */    60,   61,   62,   63,   64,   65,   66,   67,   68,   69,
 /*    70 */    70,   71,   72,   79,   56,   57,   58,   83,   84,   85,
 /*    80 */    86,   87,   88,   89,   90,   91,   92,   93,   94,   95,
 /*    90 */    96,   97,   98,   99,  100,   78,    0,  103,  104,   82,
 /*   100 */   106,  107,  108,  101,  102,    7,    8,   81,   10,   11,
 /*   110 */    12,   13,   16,   15,   78,   79,  107,  108,   82,   83,
 /*   120 */    84,   85,   86,   87,   88,   89,   90,   91,   92,   93,
 /*   130 */    94,   95,   96,   97,   98,   99,  100,  111,  112,  103,
 /*   140 */   104,    8,   14,   10,   16,   12,   13,    3,    4,  111,
 /*   150 */   112,   69,   54,   55,   54,   55,   82,   59,   60,   61,
 /*   160 */    62,   63,   64,   65,   66,   67,   68,   69,   70,   71,
 /*   170 */    72,   36,   37,   38,   39,   40,   41,   42,   43,   44,
 /*   180 */    45,   46,   47,   48,   49,   50,   82,   54,   55,   56,
 /*   190 */    34,   35,   59,   60,   61,   62,   63,   64,   65,   66,
 /*   200 */    67,   68,   69,   70,   71,   72,   80,    0,   75,   83,
 /*   210 */    84,   85,   86,   87,   88,   89,   90,   91,   92,   93,
 /*   220 */    94,   95,   96,   97,   98,   99,  100,    9,    9,  103,
 /*   230 */   104,   30,   16,   16,   16,   16,  110,   83,   84,   85,
 /*   240 */    86,   87,   88,   89,   90,   91,   92,   93,   94,   95,
 /*   250 */    96,   97,   98,   99,  100,   60,   61,  103,  104,   69,
 /*   260 */    14,   54,   16,   41,  110,   67,   56,   31,   12,   30,
 /*   270 */    33,   83,   84,   85,   86,   87,   88,   89,   90,   91,
 /*   280 */    92,   93,   94,   95,   96,   97,   98,   99,  100,   73,
 /*   290 */    73,  103,  104,   32,   30,  113,  113,  109,   83,   84,
 /*   300 */    85,   86,   87,   88,   89,   90,   91,   92,   93,   94,
 /*   310 */    95,   96,   97,   98,   99,  100,    5,   79,  103,  104,
 /*   320 */   105,   83,   84,   85,   86,   87,   88,   89,   90,   91,
 /*   330 */    92,   93,   94,   95,   96,   97,   98,   99,  100,  113,
 /*   340 */    29,  103,  104,  113,   79,  113,  113,  113,   83,   84,
 /*   350 */    85,   86,   87,   88,   89,   90,   91,   92,   93,   94,
 /*   360 */    95,   96,   97,   98,   99,  100,   79,  113,  103,  104,
 /*   370 */    83,   84,   85,   86,   87,   88,   89,   90,   91,   92,
 /*   380 */    93,   94,   95,   96,   97,   98,   99,  100,  113,  113,
 /*   390 */   103,  104,    8,  113,   10,  113,   12,   13,   83,   84,
 /*   400 */    85,   86,   87,   88,   89,   90,   91,   92,   93,   94,
 /*   410 */    95,   96,   97,   98,   99,  100,  113,  113,  103,  104,
 /*   420 */    83,   84,   85,   86,   87,   88,   89,   90,   91,   92,
 /*   430 */    93,   94,   95,   96,   97,   98,   99,  100,   54,   55,
 /*   440 */   103,  104,    0,   59,   60,   61,   62,   63,   64,   65,
 /*   450 */    66,   67,   68,   69,   70,   71,   72,    8,   16,   10,
 /*   460 */   113,   12,   13,   83,   84,   85,   86,   87,   88,   89,
 /*   470 */    90,   91,   92,   93,   94,   95,   96,   97,   98,   99,
 /*   480 */   100,  113,  113,  103,  104,   83,   84,   85,   86,   87,
 /*   490 */    88,   89,   90,   91,   92,   93,   94,   95,   96,   97,
 /*   500 */    98,   99,  100,   54,   55,  103,  104,  113,   59,   60,
 /*   510 */    61,   62,   63,   64,   65,   66,   67,   68,   69,   70,
 /*   520 */    71,   72,   83,   84,   85,   86,   87,   88,   89,   90,
 /*   530 */    91,   92,   93,   94,   95,   96,   97,   98,   99,  100,
 /*   540 */   113,  113,  103,  104,  113,   83,   84,   85,   86,   87,
 /*   550 */    88,   89,   90,   91,   92,   93,   94,   95,   96,   97,
 /*   560 */    98,   99,  100,  113,  113,  103,  104,   83,   84,   85,
 /*   570 */    86,   87,   88,   89,   90,   91,   92,   93,   94,   95,
 /*   580 */    96,   97,   98,   99,  100,  113,  113,  103,  104,   83,
 /*   590 */    84,   85,   86,   87,   88,   89,   90,   91,   92,   93,
 /*   600 */    94,   95,   96,   97,   98,   99,  100,  113,  113,  103,
 /*   610 */   104,   83,   84,   85,   86,   87,   88,   89,   90,   91,
 /*   620 */    92,   93,   94,   95,   96,   97,   98,   99,  100,  113,
 /*   630 */   113,  103,  104,   83,   84,   85,   86,   87,   88,   89,
 /*   640 */    90,   91,   92,   93,   94,   95,   96,   97,   98,   99,
 /*   650 */   100,  113,  113,  103,  104,   83,   84,   85,   86,   87,
 /*   660 */    88,   89,   90,   91,   92,   93,   94,   95,   96,   97,
 /*   670 */    98,   99,  100,  113,  113,  103,  104,   83,   84,   85,
 /*   680 */    86,   87,   88,   89,   90,   91,   92,   93,   94,   95,
 /*   690 */    96,   97,   98,   99,  100,  113,  113,  103,  104,   83,
 /*   700 */    84,   85,   86,   87,   88,   89,   90,   91,   92,   93,
 /*   710 */    94,   95,   96,   97,   98,   99,  100,  113,  113,  103,
 /*   720 */   104,   83,   84,   85,   86,   87,   88,   89,   90,   91,
 /*   730 */    92,   93,   94,   95,   96,   97,   98,   99,  100,  113,
 /*   740 */   113,  103,  104,   83,   84,   85,   86,   87,   88,   89,
 /*   750 */    90,   91,   92,   93,   94,   95,   96,   97,   98,   99,
 /*   760 */   100,  113,  113,  103,  104,   83,   84,   85,   86,   87,
 /*   770 */    88,   89,   90,   91,   92,   93,   94,   95,   96,   97,
 /*   780 */    98,   99,  100,  113,  113,  103,  104,   83,   84,   85,
 /*   790 */    86,   87,   88,   89,   90,   91,   92,   93,   94,   95,
 /*   800 */    96,   97,   98,   99,  100,  113,  113,  103,  104,   83,
 /*   810 */    84,   85,   86,   87,   88,   89,   90,   91,   92,   93,
 /*   820 */    94,   95,   96,   97,   98,   99,  100,  113,   83,  103,
 /*   830 */   104,   86,  113,   88,   89,   90,   91,   92,   93,   94,
 /*   840 */    95,   96,   97,   98,   99,  100,  113,   83,  103,  104,
 /*   850 */    86,  113,  113,   89,   90,   91,   92,   93,   94,   95,
 /*   860 */    96,   97,   98,   99,  100,  113,   83,  103,  104,   86,
 /*   870 */   113,  113,   89,   90,   91,   92,   93,   94,   95,   96,
 /*   880 */    97,   98,   99,  100,    8,   83,  103,  104,   86,  113,
 /*   890 */   113,  113,   90,   91,   92,   93,   94,   95,   96,   97,
 /*   900 */    98,   99,  100,  113,   83,  103,  104,   86,  113,  113,
 /*   910 */   113,  113,   91,   92,   93,   94,   95,   96,   97,   98,
 /*   920 */    99,  100,  113,  113,  103,  104,  113,  113,  113,  113,
 /*   930 */    17,   18,   19,   20,   21,   22,   23,   24,   25,   26,
 /*   940 */    27,   28,   83,  113,  113,   86,  113,  113,   72,  113,
 /*   950 */    74,   92,   93,   94,   95,   96,   97,   98,   99,  100,
 /*   960 */   113,  113,  103,  104,  113,   83,  113,  113,   86,  113,
 /*   970 */   113,  113,  113,   60,   61,   93,   94,   95,   96,   97,
 /*   980 */    98,   99,  100,  113,   83,  103,  104,   86,  113,  113,
 /*   990 */   113,  113,  113,  113,   93,   94,   95,   96,   97,   98,
 /*  1000 */    99,  100,  113,  113,  103,  104,  113,  113,    3,    4,
 /*  1010 */     5,    6,    7,    8,    9,   10,   11,   12,   13,    0,
 /*  1020 */    15,  113,    3,    4,    5,    6,    7,    8,  113,   10,
 /*  1030 */    11,   12,   13,  113,   15,   83,  113,  113,   86,  113,
 /*  1040 */   113,  113,  113,  113,  113,  113,   94,   95,   96,   97,
 /*  1050 */    98,   99,  100,  113,  113,  103,  104,   83,  113,  113,
 /*  1060 */    86,  113,  113,  113,  113,  113,  113,  113,   94,   95,
 /*  1070 */    96,   97,   98,   99,  100,  113,  113,  103,  104,   83,
 /*  1080 */   113,  113,   86,  113,  113,  113,  113,  113,  113,  113,
 /*  1090 */    94,   95,   96,   97,   98,   99,  100,  113,   83,  103,
 /*  1100 */   104,   86,  113,  113,  113,  113,  113,  113,  113,   94,
 /*  1110 */    95,   96,   97,   98,   99,  100,  113,  113,  103,  104,
 /*  1120 */   113,  113,   83,  113,  113,   86,  113,    8,  113,  113,
 /*  1130 */   113,   12,   13,   94,   95,   96,   97,   98,   99,  100,
 /*  1140 */   113,   83,  103,  104,   86,  113,  113,  113,  113,  113,
 /*  1150 */   113,  113,   94,   95,   96,   97,   98,   99,  100,  113,
 /*  1160 */   113,  103,  104,   83,  113,  113,   86,  113,  113,  113,
 /*  1170 */   113,  113,  113,  113,   94,   95,   96,   97,   98,   99,
 /*  1180 */   100,  113,   83,  103,  104,   86,   67,   68,   69,   70,
 /*  1190 */    71,   72,  113,   94,   95,   96,   97,   98,   99,  100,
 /*  1200 */   113,  113,  103,  104,  113,  113,   83,  113,  113,   86,
 /*  1210 */   113,  113,  113,  113,  113,  113,  113,   94,   95,   96,
 /*  1220 */    97,   98,   99,  100,  113,   83,  103,  104,   86,  113,
 /*  1230 */   113,  113,  113,  113,  113,  113,   94,   95,   96,   97,
 /*  1240 */    98,   99,  100,  113,  113,  103,  104,   83,  113,  113,
 /*  1250 */    86,  113,  113,  113,  113,  113,  113,  113,   94,   95,
 /*  1260 */    96,   97,   98,   99,  100,  113,   83,  103,  104,   86,
 /*  1270 */   113,  113,  113,  113,  113,  113,  113,   94,   95,   96,
 /*  1280 */    97,   98,   99,  100,  113,  113,  103,  104,  113,  113,
 /*  1290 */    83,  113,  113,   86,  113,  113,  113,  113,  113,  113,
 /*  1300 */   113,   94,   95,   96,   97,   98,   99,  100,  113,   83,
 /*  1310 */   103,  104,   86,  113,  113,  113,  113,  113,  113,  113,
 /*  1320 */    94,   95,   96,   97,   98,   99,  100,  113,  113,  103,
 /*  1330 */   104,   83,  113,  113,   86,  113,  113,  113,  113,  113,
 /*  1340 */   113,  113,   94,   95,   96,   97,   98,   99,  100,  113,
 /*  1350 */    83,  103,  104,   86,  113,  113,  113,  113,  113,  113,
 /*  1360 */   113,  113,   95,   96,   97,   98,   99,  100,  113,  113,
 /*  1370 */   103,  104,  113,  113,   83,  113,  113,   86,  113,  113,
 /*  1380 */   113,  113,  113,  113,  113,  113,   95,   96,   97,   98,
 /*  1390 */    99,  100,  113,   83,  103,  104,   86,  113,  113,  113,
 /*  1400 */   113,  113,  113,  113,  113,   95,   96,   97,   98,   99,
 /*  1410 */   100,  113,  113,  103,  104,  113,  113,   82,   83,  113,
 /*  1420 */   113,   86,  113,  113,  113,  113,  113,   83,  113,  113,
 /*  1430 */    86,  113,   97,   98,   99,  100,  113,  113,  103,  104,
 /*  1440 */    96,   97,   98,   99,  100,  113,   83,  103,  104,   86,
 /*  1450 */   113,  113,  113,  113,   83,  113,  113,   86,  113,   96,
 /*  1460 */    97,   98,   99,  100,  113,  113,  103,  104,   97,   98,
 /*  1470 */    99,  100,  113,   83,  103,  104,   86,  113,  113,  113,
 /*  1480 */   113,  113,  113,  113,  113,  113,  113,   97,   98,   99,
 /*  1490 */   100,  113,   83,  103,  104,   86,  113,  113,  113,  113,
 /*  1500 */   113,   83,  113,  113,   86,  113,   97,   98,   99,  100,
 /*  1510 */   113,   83,  103,  104,   86,   97,   98,   99,  100,  113,
 /*  1520 */   113,  103,  104,  113,  113,   97,   98,   99,  100,  113,
 /*  1530 */    83,  103,  104,   86,  113,  113,  113,  113,   83,  113,
 /*  1540 */   113,   86,  113,  113,   97,   98,   99,  100,  113,  113,
 /*  1550 */   103,  104,   97,   98,   99,  100,  113,   83,  103,  104,
 /*  1560 */    86,  113,  113,  113,  113,  113,  113,  113,  113,  113,
 /*  1570 */   113,   97,   98,   99,  100,  113,   83,  103,  104,   86,
 /*  1580 */   113,  113,  113,  113,  113,   83,  113,  113,   86,  113,
 /*  1590 */    97,   98,   99,  100,  113,   83,  103,  104,   86,   97,
 /*  1600 */    98,   99,  100,  113,  113,  103,  104,  113,  113,   97,
 /*  1610 */    98,   99,  100,  113,   83,  103,  104,   86,  113,  113,
 /*  1620 */   113,  113,   83,  113,  113,   86,  113,  113,   97,   98,
 /*  1630 */    99,  100,  113,  113,  103,  104,   97,   98,   99,  100,
 /*  1640 */   113,   83,  103,  104,   86,  113,  113,  113,  113,  113,
 /*  1650 */   113,  113,  113,  113,  113,   97,   98,   99,  100,  113,
 /*  1660 */   113,  103,  104,
};
#define YY_SHIFT_COUNT    (145)
#define YY_SHIFT_MIN      (0)
#define YY_SHIFT_MAX      (1119)
static const unsigned short int yy_shift_ofst[] = {
 /*     0 */     0,  384,   98,  133,  133,  449,  449,  449,  449,  449,
 /*    10 */   449,  449,  449,  449,  449,  449,  449,  449,  449,  449,
 /*    20 */   449,  449,  449,  449,  449,  449,  449,  449,  449,  449,
 /*    30 */   449,  449,  449,  449,  449,  449,  449,  449,  449,  449,
 /*    40 */   449,  449,  449,  449,  449,  449,  449,  449,  449,  449,
 /*    50 */   449,  449,  449,  449,   98,  449,  449,  449,  449,  449,
 /*    60 */   449,  449,  449,  449,  449,  449,  449,  449,  449,  449,
 /*    70 */   449, 1119,   34,   82,   37,  876,   34,   82, 1005, 1019,
 /*    80 */    37,   37,   37,   37,   37,   37,   37,  135,  135,  135,
 /*    90 */   913,    5,    5,    5,    5,    5,    5,    5,    5,    5,
 /*   100 */     5,    5,    5,    5,   18,    5,   18,    5,   18,    5,
 /*   110 */   207,   96,  442,  144,  218,  128,  216,  100,  100,  219,
 /*   120 */   100,  156,  217,  195,  100,  156,  144,  311,  246,  201,
 /*   130 */     8,  190,  222,  198,  210,  236,  256,  239,  237,  261,
 /*   140 */   236,  237,  261,  236,  264,    8,
};
#define YY_REDUCE_COUNT (86)
#define YY_REDUCE_MIN   (-103)
#define YY_REDUCE_MAX   (1558)
static const short yy_reduce_ofst[] = {
 /*     0 */   -61,   -6,   36,  126,  154,  188,  215,  238,  265,  287,
 /*    10 */   315,  337,  380,  402,  439,  462,  484,  506,  528,  550,
 /*    20 */   572,  594,  616,  638,  660,  682,  704,  726,  745,  764,
 /*    30 */   783,  802,  821,  859,  882,  901,  952,  974,  996, 1015,
 /*    40 */  1039, 1058, 1080, 1099, 1123, 1142, 1164, 1183, 1207, 1226,
 /*    50 */  1248, 1267, 1291, 1310, 1335, 1344, 1363, 1371, 1390, 1409,
 /*    60 */  1418, 1428, 1447, 1455, 1474, 1493, 1502, 1512, 1531, 1539,
 /*    70 */  1558,  -63,   26, -103,   17,    2,   38,    9,  -82,  -82,
 /*    80 */   -76,  -73,  -68,  -31,  -29,   74,  104,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   361,  437,  361,  444,  446,  441,  434,  361,  361,  361,
 /*    10 */   361,  361,  361,  361,  361,  361,  361,  361,  361,  361,
 /*    20 */   361,  361,  361,  361,  361,  361,  361,  361,  361,  361,
 /*    30 */   361,  361,  361,  361,  361,  361,  361,  361,  361,  361,
 /*    40 */   361,  361,  361,  361,  361,  361,  361,  361,  361,  361,
 /*    50 */   361,  361,  361,  361,  361,  361,  361,  361,  361,  361,
 /*    60 */   361,  361,  361,  361,  361,  361,  361,  361,  361,  361,
 /*    70 */   361,  361,  494,  437,  361,  477,  361,  361,  361,  361,
 /*    80 */   361,  361,  361,  361,  361,  361,  361,  469,  395,  394,
 /*    90 */   475,  410,  409,  408,  407,  406,  405,  404,  403,  402,
 /*   100 */   401,  400,  399,  470,  472,  398,  415,  397,  414,  396,
 /*   110 */   361,  361,  361,  388,  361,  361,  361,  471,  413,  361,
 /*   120 */   412,  468,  361,  475,  411,  393,  464,  463,  361,  486,
 /*   130 */   482,  361,  361,  361,  496,  390,  361,  361,  467,  466,
 /*   140 */   465,  392,  391,  389,  361,  361,
};
/********** End of lemon-generated parsing tables *****************************/

/* The next table maps tokens (terminal symbols) into fallback tokens.  
** If a construct like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
**
** This feature can be used, for example, to cause some keywords in a language
** to revert to identifiers if they keyword does not apply in the context where
** it appears.
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
**
** After the "shift" half of a SHIFTREDUCE action, the stateno field
** actually contains the reduce action for the second half of the
** SHIFTREDUCE.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number, or reduce action in SHIFTREDUCE */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  yyStackEntry *yytos;          /* Pointer to top element of the stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyhwm;                    /* High-water mark of the stack */
#endif
#ifndef YYNOERRORRECOVERY
  int yyerrcnt;                 /* Shifts left before out of the error */
#endif
  grn_expr_parserARG_SDECL                /* A place to hold %extra_argument */
  grn_expr_parserCTX_SDECL                /* A place to hold %extra_context */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
  yyStackEntry yystk0;          /* First stack entry */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
  yyStackEntry *yystackEnd;            /* Last entry in the stack */
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

#if defined(YYCOVERAGE) || !defined(NDEBUG)
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  /*    0 */ "$",
  /*    1 */ "START_OUTPUT_COLUMNS",
  /*    2 */ "START_ADJUSTER",
  /*    3 */ "LOGICAL_AND",
  /*    4 */ "LOGICAL_AND_NOT",
  /*    5 */ "LOGICAL_OR",
  /*    6 */ "NEGATIVE",
  /*    7 */ "QSTRING",
  /*    8 */ "PARENL",
  /*    9 */ "PARENR",
  /*   10 */ "ADJUST",
  /*   11 */ "RELATIVE_OP",
  /*   12 */ "IDENTIFIER",
  /*   13 */ "BRACEL",
  /*   14 */ "BRACER",
  /*   15 */ "EVAL",
  /*   16 */ "COMMA",
  /*   17 */ "ASSIGN",
  /*   18 */ "STAR_ASSIGN",
  /*   19 */ "SLASH_ASSIGN",
  /*   20 */ "MOD_ASSIGN",
  /*   21 */ "PLUS_ASSIGN",
  /*   22 */ "MINUS_ASSIGN",
  /*   23 */ "SHIFTL_ASSIGN",
  /*   24 */ "SHIFTR_ASSIGN",
  /*   25 */ "SHIFTRR_ASSIGN",
  /*   26 */ "AND_ASSIGN",
  /*   27 */ "XOR_ASSIGN",
  /*   28 */ "OR_ASSIGN",
  /*   29 */ "QUESTION",
  /*   30 */ "COLON",
  /*   31 */ "BITWISE_OR",
  /*   32 */ "BITWISE_XOR",
  /*   33 */ "BITWISE_AND",
  /*   34 */ "EQUAL",
  /*   35 */ "NOT_EQUAL",
  /*   36 */ "LESS",
  /*   37 */ "GREATER",
  /*   38 */ "LESS_EQUAL",
  /*   39 */ "GREATER_EQUAL",
  /*   40 */ "IN",
  /*   41 */ "MATCH",
  /*   42 */ "NEAR",
  /*   43 */ "NEAR2",
  /*   44 */ "SIMILAR",
  /*   45 */ "TERM_EXTRACT",
  /*   46 */ "QUORUM",
  /*   47 */ "LCP",
  /*   48 */ "PREFIX",
  /*   49 */ "SUFFIX",
  /*   50 */ "REGEXP",
  /*   51 */ "SHIFTL",
  /*   52 */ "SHIFTR",
  /*   53 */ "SHIFTRR",
  /*   54 */ "PLUS",
  /*   55 */ "MINUS",
  /*   56 */ "STAR",
  /*   57 */ "SLASH",
  /*   58 */ "MOD",
  /*   59 */ "DELETE",
  /*   60 */ "INCR",
  /*   61 */ "DECR",
  /*   62 */ "NOT",
  /*   63 */ "BITWISE_NOT",
  /*   64 */ "EXACT",
  /*   65 */ "PARTIAL",
  /*   66 */ "UNSPLIT",
  /*   67 */ "DECIMAL",
  /*   68 */ "HEX_INTEGER",
  /*   69 */ "STRING",
  /*   70 */ "BOOLEAN",
  /*   71 */ "NULL",
  /*   72 */ "BRACKETL",
  /*   73 */ "BRACKETR",
  /*   74 */ "DOT",
  /*   75 */ "NONEXISTENT_COLUMN",
  /*   76 */ "suppress_unused_variable_warning",
  /*   77 */ "input",
  /*   78 */ "query",
  /*   79 */ "expression",
  /*   80 */ "output_columns",
  /*   81 */ "adjuster",
  /*   82 */ "query_element",
  /*   83 */ "primary_expression",
  /*   84 */ "assignment_expression",
  /*   85 */ "conditional_expression",
  /*   86 */ "lefthand_side_expression",
  /*   87 */ "logical_or_expression",
  /*   88 */ "logical_and_expression",
  /*   89 */ "bitwise_or_expression",
  /*   90 */ "bitwise_xor_expression",
  /*   91 */ "bitwise_and_expression",
  /*   92 */ "equality_expression",
  /*   93 */ "relational_expression",
  /*   94 */ "shift_expression",
  /*   95 */ "additive_expression",
  /*   96 */ "multiplicative_expression",
  /*   97 */ "unary_expression",
  /*   98 */ "postfix_expression",
  /*   99 */ "call_expression",
  /*  100 */ "member_expression",
  /*  101 */ "arguments",
  /*  102 */ "member_expression_part",
  /*  103 */ "object_literal",
  /*  104 */ "array_literal",
  /*  105 */ "element_list",
  /*  106 */ "property_name_and_value_list",
  /*  107 */ "property_name_and_value",
  /*  108 */ "property_name",
  /*  109 */ "argument_list",
  /*  110 */ "output_column",
  /*  111 */ "adjust_expression",
  /*  112 */ "adjust_match_expression",
};
#endif /* defined(YYCOVERAGE) || !defined(NDEBUG) */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "query ::= query query_element",
 /*   1 */ "query ::= query LOGICAL_AND query_element",
 /*   2 */ "query ::= query LOGICAL_AND_NOT query_element",
 /*   3 */ "query ::= query LOGICAL_OR query_element",
 /*   4 */ "query ::= query NEGATIVE query_element",
 /*   5 */ "query_element ::= ADJUST query_element",
 /*   6 */ "query_element ::= RELATIVE_OP query_element",
 /*   7 */ "query_element ::= IDENTIFIER RELATIVE_OP query_element",
 /*   8 */ "query_element ::= BRACEL expression BRACER",
 /*   9 */ "query_element ::= EVAL primary_expression",
 /*  10 */ "expression ::= expression COMMA assignment_expression",
 /*  11 */ "assignment_expression ::= lefthand_side_expression ASSIGN assignment_expression",
 /*  12 */ "assignment_expression ::= lefthand_side_expression STAR_ASSIGN assignment_expression",
 /*  13 */ "assignment_expression ::= lefthand_side_expression SLASH_ASSIGN assignment_expression",
 /*  14 */ "assignment_expression ::= lefthand_side_expression MOD_ASSIGN assignment_expression",
 /*  15 */ "assignment_expression ::= lefthand_side_expression PLUS_ASSIGN assignment_expression",
 /*  16 */ "assignment_expression ::= lefthand_side_expression MINUS_ASSIGN assignment_expression",
 /*  17 */ "assignment_expression ::= lefthand_side_expression SHIFTL_ASSIGN assignment_expression",
 /*  18 */ "assignment_expression ::= lefthand_side_expression SHIFTR_ASSIGN assignment_expression",
 /*  19 */ "assignment_expression ::= lefthand_side_expression SHIFTRR_ASSIGN assignment_expression",
 /*  20 */ "assignment_expression ::= lefthand_side_expression AND_ASSIGN assignment_expression",
 /*  21 */ "assignment_expression ::= lefthand_side_expression XOR_ASSIGN assignment_expression",
 /*  22 */ "assignment_expression ::= lefthand_side_expression OR_ASSIGN assignment_expression",
 /*  23 */ "conditional_expression ::= logical_or_expression QUESTION assignment_expression COLON assignment_expression",
 /*  24 */ "logical_or_expression ::= logical_or_expression LOGICAL_OR logical_and_expression",
 /*  25 */ "logical_and_expression ::= logical_and_expression LOGICAL_AND bitwise_or_expression",
 /*  26 */ "logical_and_expression ::= logical_and_expression LOGICAL_AND_NOT bitwise_or_expression",
 /*  27 */ "bitwise_or_expression ::= bitwise_or_expression BITWISE_OR bitwise_xor_expression",
 /*  28 */ "bitwise_xor_expression ::= bitwise_xor_expression BITWISE_XOR bitwise_and_expression",
 /*  29 */ "bitwise_and_expression ::= bitwise_and_expression BITWISE_AND equality_expression",
 /*  30 */ "equality_expression ::= equality_expression EQUAL relational_expression",
 /*  31 */ "equality_expression ::= equality_expression NOT_EQUAL relational_expression",
 /*  32 */ "relational_expression ::= relational_expression LESS shift_expression",
 /*  33 */ "relational_expression ::= relational_expression GREATER shift_expression",
 /*  34 */ "relational_expression ::= relational_expression LESS_EQUAL shift_expression",
 /*  35 */ "relational_expression ::= relational_expression GREATER_EQUAL shift_expression",
 /*  36 */ "relational_expression ::= relational_expression IN shift_expression",
 /*  37 */ "relational_expression ::= relational_expression MATCH shift_expression",
 /*  38 */ "relational_expression ::= relational_expression NEAR shift_expression",
 /*  39 */ "relational_expression ::= relational_expression NEAR2 shift_expression",
 /*  40 */ "relational_expression ::= relational_expression SIMILAR shift_expression",
 /*  41 */ "relational_expression ::= relational_expression TERM_EXTRACT shift_expression",
 /*  42 */ "relational_expression ::= relational_expression QUORUM shift_expression",
 /*  43 */ "relational_expression ::= relational_expression LCP shift_expression",
 /*  44 */ "relational_expression ::= relational_expression PREFIX shift_expression",
 /*  45 */ "relational_expression ::= relational_expression SUFFIX shift_expression",
 /*  46 */ "relational_expression ::= relational_expression REGEXP shift_expression",
 /*  47 */ "shift_expression ::= shift_expression SHIFTL additive_expression",
 /*  48 */ "shift_expression ::= shift_expression SHIFTR additive_expression",
 /*  49 */ "shift_expression ::= shift_expression SHIFTRR additive_expression",
 /*  50 */ "additive_expression ::= additive_expression PLUS multiplicative_expression",
 /*  51 */ "additive_expression ::= additive_expression MINUS multiplicative_expression",
 /*  52 */ "multiplicative_expression ::= multiplicative_expression STAR unary_expression",
 /*  53 */ "multiplicative_expression ::= multiplicative_expression SLASH unary_expression",
 /*  54 */ "multiplicative_expression ::= multiplicative_expression MOD unary_expression",
 /*  55 */ "unary_expression ::= DELETE unary_expression",
 /*  56 */ "unary_expression ::= INCR unary_expression",
 /*  57 */ "unary_expression ::= DECR unary_expression",
 /*  58 */ "unary_expression ::= PLUS unary_expression",
 /*  59 */ "unary_expression ::= MINUS unary_expression",
 /*  60 */ "unary_expression ::= NOT unary_expression",
 /*  61 */ "unary_expression ::= BITWISE_NOT unary_expression",
 /*  62 */ "unary_expression ::= ADJUST unary_expression",
 /*  63 */ "unary_expression ::= EXACT unary_expression",
 /*  64 */ "unary_expression ::= PARTIAL unary_expression",
 /*  65 */ "unary_expression ::= UNSPLIT unary_expression",
 /*  66 */ "postfix_expression ::= lefthand_side_expression INCR",
 /*  67 */ "postfix_expression ::= lefthand_side_expression DECR",
 /*  68 */ "call_expression ::= member_expression arguments",
 /*  69 */ "array_literal ::= BRACKETL element_list BRACKETR",
 /*  70 */ "element_list ::=",
 /*  71 */ "element_list ::= assignment_expression",
 /*  72 */ "object_literal ::= BRACEL property_name_and_value_list BRACER",
 /*  73 */ "property_name_and_value_list ::=",
 /*  74 */ "property_name_and_value ::= property_name COLON assignment_expression",
 /*  75 */ "member_expression_part ::= BRACKETL expression BRACKETR",
 /*  76 */ "arguments ::= PARENL argument_list PARENR",
 /*  77 */ "argument_list ::=",
 /*  78 */ "argument_list ::= assignment_expression",
 /*  79 */ "argument_list ::= argument_list COMMA assignment_expression",
 /*  80 */ "output_columns ::=",
 /*  81 */ "output_columns ::= output_column",
 /*  82 */ "output_columns ::= output_columns COMMA",
 /*  83 */ "output_columns ::= output_columns COMMA output_column",
 /*  84 */ "output_column ::= STAR",
 /*  85 */ "output_column ::= NONEXISTENT_COLUMN",
 /*  86 */ "output_column ::= assignment_expression",
 /*  87 */ "adjuster ::= adjuster PLUS adjust_expression",
 /*  88 */ "adjust_expression ::= adjust_match_expression STAR DECIMAL",
 /*  89 */ "adjust_match_expression ::= IDENTIFIER MATCH STRING",
 /*  90 */ "input ::= query",
 /*  91 */ "input ::= expression",
 /*  92 */ "input ::= START_OUTPUT_COLUMNS output_columns",
 /*  93 */ "input ::= START_ADJUSTER adjuster",
 /*  94 */ "query ::= query_element",
 /*  95 */ "query_element ::= QSTRING",
 /*  96 */ "query_element ::= PARENL query PARENR",
 /*  97 */ "expression ::= assignment_expression",
 /*  98 */ "assignment_expression ::= conditional_expression",
 /*  99 */ "conditional_expression ::= logical_or_expression",
 /* 100 */ "logical_or_expression ::= logical_and_expression",
 /* 101 */ "logical_and_expression ::= bitwise_or_expression",
 /* 102 */ "bitwise_or_expression ::= bitwise_xor_expression",
 /* 103 */ "bitwise_xor_expression ::= bitwise_and_expression",
 /* 104 */ "bitwise_and_expression ::= equality_expression",
 /* 105 */ "equality_expression ::= relational_expression",
 /* 106 */ "relational_expression ::= shift_expression",
 /* 107 */ "shift_expression ::= additive_expression",
 /* 108 */ "additive_expression ::= multiplicative_expression",
 /* 109 */ "multiplicative_expression ::= unary_expression",
 /* 110 */ "unary_expression ::= postfix_expression",
 /* 111 */ "postfix_expression ::= lefthand_side_expression",
 /* 112 */ "lefthand_side_expression ::= call_expression",
 /* 113 */ "lefthand_side_expression ::= member_expression",
 /* 114 */ "member_expression ::= primary_expression",
 /* 115 */ "member_expression ::= member_expression member_expression_part",
 /* 116 */ "primary_expression ::= object_literal",
 /* 117 */ "primary_expression ::= PARENL expression PARENR",
 /* 118 */ "primary_expression ::= IDENTIFIER",
 /* 119 */ "primary_expression ::= array_literal",
 /* 120 */ "primary_expression ::= DECIMAL",
 /* 121 */ "primary_expression ::= HEX_INTEGER",
 /* 122 */ "primary_expression ::= STRING",
 /* 123 */ "primary_expression ::= BOOLEAN",
 /* 124 */ "primary_expression ::= NULL",
 /* 125 */ "element_list ::= element_list COMMA assignment_expression",
 /* 126 */ "property_name_and_value_list ::= property_name_and_value",
 /* 127 */ "property_name_and_value_list ::= property_name_and_value_list COMMA property_name_and_value",
 /* 128 */ "property_name ::= STRING",
 /* 129 */ "member_expression_part ::= DOT IDENTIFIER",
 /* 130 */ "adjuster ::=",
 /* 131 */ "adjuster ::= adjust_expression",
 /* 132 */ "adjust_expression ::= adjust_match_expression",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.  Return the number
** of errors.  Return 0 on success.
*/
static int yyGrowStack(yyParser *p){
  int newSize;
  int idx;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  idx = p->yytos ? (int)(p->yytos - p->yystack) : 0;
  if( p->yystack==&p->yystk0 ){
    pNew = malloc(newSize*sizeof(pNew[0]));
    if( pNew ) pNew[0] = p->yystk0;
  }else{
    pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  }
  if( pNew ){
    p->yystack = pNew;
    p->yytos = &p->yystack[idx];
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows from %d to %d entries.\n",
              yyTracePrompt, p->yystksz, newSize);
    }
#endif
    p->yystksz = newSize;
  }
  return pNew==0; 
}
#endif

/* Datatype of the argument to the memory allocated passed as the
** second argument to grn_expr_parserAlloc() below.  This can be changed by
** putting an appropriate #define in the %include section of the input
** grammar.
*/
#ifndef YYMALLOCARGTYPE
# define YYMALLOCARGTYPE size_t
#endif

/* Initialize a new parser that has already been allocated.
*/
void grn_expr_parserInit(void *yypRawParser grn_expr_parserCTX_PDECL){
  yyParser *yypParser = (yyParser*)yypRawParser;
  grn_expr_parserCTX_STORE
#ifdef YYTRACKMAXSTACKDEPTH
  yypParser->yyhwm = 0;
#endif
#if YYSTACKDEPTH<=0
  yypParser->yytos = NULL;
  yypParser->yystack = NULL;
  yypParser->yystksz = 0;
  if( yyGrowStack(yypParser) ){
    yypParser->yystack = &yypParser->yystk0;
    yypParser->yystksz = 1;
  }
#endif
#ifndef YYNOERRORRECOVERY
  yypParser->yyerrcnt = -1;
#endif
  yypParser->yytos = yypParser->yystack;
  yypParser->yystack[0].stateno = 0;
  yypParser->yystack[0].major = 0;
#if YYSTACKDEPTH>0
  yypParser->yystackEnd = &yypParser->yystack[YYSTACKDEPTH-1];
#endif
}

#ifndef grn_expr_parser_ENGINEALWAYSONSTACK
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
void *grn_expr_parserAlloc(void *(*mallocProc)(YYMALLOCARGTYPE) grn_expr_parserCTX_PDECL){
  yyParser *yypParser;
  yypParser = (yyParser*)(*mallocProc)( (YYMALLOCARGTYPE)sizeof(yyParser) );
  if( yypParser ){
    grn_expr_parserCTX_STORE
    grn_expr_parserInit(yypParser grn_expr_parserCTX_PARAM);
  }
  return (void*)yypParser;
}
#endif /* grn_expr_parser_ENGINEALWAYSONSTACK */


/* The following function deletes the "minor type" or semantic value
** associated with a symbol.  The symbol can be either a terminal
** or nonterminal. "yymajor" is the symbol code, and "yypminor" is
** a pointer to the value to be deleted.  The code used to do the 
** deletions is derived from the %destructor and/or %token_destructor
** directives of the input grammar.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  grn_expr_parserARG_FETCH
  grn_expr_parserCTX_FETCH
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are *not* used
    ** inside the C code.
    */
/********* Begin destructor definitions ***************************************/
    case 76: /* suppress_unused_variable_warning */
{
#line 14 "../../groonga/lib/grn_ecmascript.lemon"

  (void)efsi;

#line 1076 "../../groonga/lib/grn_ecmascript.c"
}
      break;
/********* End destructor definitions *****************************************/
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
*/
static void yy_pop_parser_stack(yyParser *pParser){
  yyStackEntry *yytos;
  assert( pParser->yytos!=0 );
  assert( pParser->yytos > pParser->yystack );
  yytos = pParser->yytos--;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yy_destructor(pParser, yytos->major, &yytos->minor);
}

/*
** Clear all secondary memory allocations from the parser
*/
void grn_expr_parserFinalize(void *p){
  yyParser *pParser = (yyParser*)p;
  while( pParser->yytos>pParser->yystack ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  if( pParser->yystack!=&pParser->yystk0 ) free(pParser->yystack);
#endif
}

#ifndef grn_expr_parser_ENGINEALWAYSONSTACK
/* 
** Deallocate and destroy a parser.  Destructors are called for
** all stack elements before shutting the parser down.
**
** If the YYPARSEFREENEVERNULL macro exists (for example because it
** is defined in a %include section of the input grammar) then it is
** assumed that the input pointer is never NULL.
*/
void grn_expr_parserFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
#ifndef YYPARSEFREENEVERNULL
  if( p==0 ) return;
#endif
  grn_expr_parserFinalize(p);
  (*freeProc)(p);
}
#endif /* grn_expr_parser_ENGINEALWAYSONSTACK */

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int grn_expr_parserStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyhwm;
}
#endif

/* This array of booleans keeps track of the parser statement
** coverage.  The element yycoverage[X][Y] is set when the parser
** is in state X and has a lookahead token Y.  In a well-tested
** systems, every element of this matrix should end up being set.
*/
#if defined(YYCOVERAGE)
static unsigned char yycoverage[YYNSTATE][YYNTOKEN];
#endif

/*
** Write into out a description of every state/lookahead combination that
**
**   (1)  has not been used by the parser, and
**   (2)  is not a syntax error.
**
** Return the number of missed state/lookahead combinations.
*/
#if defined(YYCOVERAGE)
int grn_expr_parserCoverage(FILE *out){
  int stateno, iLookAhead, i;
  int nMissed = 0;
  for(stateno=0; stateno<YYNSTATE; stateno++){
    i = yy_shift_ofst[stateno];
    for(iLookAhead=0; iLookAhead<YYNTOKEN; iLookAhead++){
      if( yy_lookahead[i+iLookAhead]!=iLookAhead ) continue;
      if( yycoverage[stateno][iLookAhead]==0 ) nMissed++;
      if( out ){
        fprintf(out,"State %d lookahead %s %s\n", stateno,
                yyTokenName[iLookAhead],
                yycoverage[stateno][iLookAhead] ? "ok" : "missed");
      }
    }
  }
  return nMissed;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
*/
static YYACTIONTYPE yy_find_shift_action(
  YYCODETYPE iLookAhead,    /* The look-ahead token */
  YYACTIONTYPE stateno      /* Current state number */
){
  int i;

  if( stateno>YY_MAX_SHIFT ) return stateno;
  assert( stateno <= YY_SHIFT_COUNT );
#if defined(YYCOVERAGE)
  yycoverage[stateno][iLookAhead] = 1;
#endif
  do{
    i = yy_shift_ofst[stateno];
    assert( i>=0 );
    /* assert( i+YYNTOKEN<=(int)YY_NLOOKAHEAD ); */
    assert( iLookAhead!=YYNOCODE );
    assert( iLookAhead < YYNTOKEN );
    i += iLookAhead;
    if( i>=YY_NLOOKAHEAD || yy_lookahead[i]!=iLookAhead ){
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
        assert( yyFallback[iFallback]==0 ); /* Fallback loop must terminate */
        iLookAhead = iFallback;
        continue;
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
          j<(int)(sizeof(yy_lookahead)/sizeof(yy_lookahead[0])) &&
          yy_lookahead[j]==YYWILDCARD && iLookAhead>0
        ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead],
               yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
      return yy_default[stateno];
    }else{
      return yy_action[i];
    }
  }while(1);
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
*/
static YYACTIONTYPE yy_find_reduce_action(
  YYACTIONTYPE stateno,     /* Current state number */
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
static void yyStackOverflow(yyParser *yypParser){
   grn_expr_parserARG_FETCH
   grn_expr_parserCTX_FETCH
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
/******** Begin %stack_overflow code ******************************************/
/******** End %stack_overflow code ********************************************/
   grn_expr_parserARG_STORE /* Suppress warning about unused %extra_argument var */
   grn_expr_parserCTX_STORE
}

/*
** Print tracing information for a SHIFT action
*/
#ifndef NDEBUG
static void yyTraceShift(yyParser *yypParser, int yyNewState, const char *zTag){
  if( yyTraceFILE ){
    if( yyNewState<YYNSTATE ){
      fprintf(yyTraceFILE,"%s%s '%s', go to state %d\n",
         yyTracePrompt, zTag, yyTokenName[yypParser->yytos->major],
         yyNewState);
    }else{
      fprintf(yyTraceFILE,"%s%s '%s', pending reduce %d\n",
         yyTracePrompt, zTag, yyTokenName[yypParser->yytos->major],
         yyNewState - YY_MIN_REDUCE);
    }
  }
}
#else
# define yyTraceShift(X,Y,Z)
#endif

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  YYACTIONTYPE yyNewState,      /* The new state to shift in */
  YYCODETYPE yyMajor,           /* The major token to shift in */
  grn_expr_parserTOKENTYPE yyMinor        /* The minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yytos++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( (int)(yypParser->yytos - yypParser->yystack)>yypParser->yyhwm ){
    yypParser->yyhwm++;
    assert( yypParser->yyhwm == (int)(yypParser->yytos - yypParser->yystack) );
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yytos>yypParser->yystackEnd ){
    yypParser->yytos--;
    yyStackOverflow(yypParser);
    return;
  }
#else
  if( yypParser->yytos>=&yypParser->yystack[yypParser->yystksz] ){
    if( yyGrowStack(yypParser) ){
      yypParser->yytos--;
      yyStackOverflow(yypParser);
      return;
    }
  }
#endif
  if( yyNewState > YY_MAX_SHIFT ){
    yyNewState += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
  }
  yytos = yypParser->yytos;
  yytos->stateno = yyNewState;
  yytos->major = yyMajor;
  yytos->minor.yy0 = yyMinor;
  yyTraceShift(yypParser, yyNewState, "Shift");
}

/* For rule J, yyRuleInfoLhs[J] contains the symbol on the left-hand side
** of that rule */
static const YYCODETYPE yyRuleInfoLhs[] = {
    78,  /* (0) query ::= query query_element */
    78,  /* (1) query ::= query LOGICAL_AND query_element */
    78,  /* (2) query ::= query LOGICAL_AND_NOT query_element */
    78,  /* (3) query ::= query LOGICAL_OR query_element */
    78,  /* (4) query ::= query NEGATIVE query_element */
    82,  /* (5) query_element ::= ADJUST query_element */
    82,  /* (6) query_element ::= RELATIVE_OP query_element */
    82,  /* (7) query_element ::= IDENTIFIER RELATIVE_OP query_element */
    82,  /* (8) query_element ::= BRACEL expression BRACER */
    82,  /* (9) query_element ::= EVAL primary_expression */
    79,  /* (10) expression ::= expression COMMA assignment_expression */
    84,  /* (11) assignment_expression ::= lefthand_side_expression ASSIGN assignment_expression */
    84,  /* (12) assignment_expression ::= lefthand_side_expression STAR_ASSIGN assignment_expression */
    84,  /* (13) assignment_expression ::= lefthand_side_expression SLASH_ASSIGN assignment_expression */
    84,  /* (14) assignment_expression ::= lefthand_side_expression MOD_ASSIGN assignment_expression */
    84,  /* (15) assignment_expression ::= lefthand_side_expression PLUS_ASSIGN assignment_expression */
    84,  /* (16) assignment_expression ::= lefthand_side_expression MINUS_ASSIGN assignment_expression */
    84,  /* (17) assignment_expression ::= lefthand_side_expression SHIFTL_ASSIGN assignment_expression */
    84,  /* (18) assignment_expression ::= lefthand_side_expression SHIFTR_ASSIGN assignment_expression */
    84,  /* (19) assignment_expression ::= lefthand_side_expression SHIFTRR_ASSIGN assignment_expression */
    84,  /* (20) assignment_expression ::= lefthand_side_expression AND_ASSIGN assignment_expression */
    84,  /* (21) assignment_expression ::= lefthand_side_expression XOR_ASSIGN assignment_expression */
    84,  /* (22) assignment_expression ::= lefthand_side_expression OR_ASSIGN assignment_expression */
    85,  /* (23) conditional_expression ::= logical_or_expression QUESTION assignment_expression COLON assignment_expression */
    87,  /* (24) logical_or_expression ::= logical_or_expression LOGICAL_OR logical_and_expression */
    88,  /* (25) logical_and_expression ::= logical_and_expression LOGICAL_AND bitwise_or_expression */
    88,  /* (26) logical_and_expression ::= logical_and_expression LOGICAL_AND_NOT bitwise_or_expression */
    89,  /* (27) bitwise_or_expression ::= bitwise_or_expression BITWISE_OR bitwise_xor_expression */
    90,  /* (28) bitwise_xor_expression ::= bitwise_xor_expression BITWISE_XOR bitwise_and_expression */
    91,  /* (29) bitwise_and_expression ::= bitwise_and_expression BITWISE_AND equality_expression */
    92,  /* (30) equality_expression ::= equality_expression EQUAL relational_expression */
    92,  /* (31) equality_expression ::= equality_expression NOT_EQUAL relational_expression */
    93,  /* (32) relational_expression ::= relational_expression LESS shift_expression */
    93,  /* (33) relational_expression ::= relational_expression GREATER shift_expression */
    93,  /* (34) relational_expression ::= relational_expression LESS_EQUAL shift_expression */
    93,  /* (35) relational_expression ::= relational_expression GREATER_EQUAL shift_expression */
    93,  /* (36) relational_expression ::= relational_expression IN shift_expression */
    93,  /* (37) relational_expression ::= relational_expression MATCH shift_expression */
    93,  /* (38) relational_expression ::= relational_expression NEAR shift_expression */
    93,  /* (39) relational_expression ::= relational_expression NEAR2 shift_expression */
    93,  /* (40) relational_expression ::= relational_expression SIMILAR shift_expression */
    93,  /* (41) relational_expression ::= relational_expression TERM_EXTRACT shift_expression */
    93,  /* (42) relational_expression ::= relational_expression QUORUM shift_expression */
    93,  /* (43) relational_expression ::= relational_expression LCP shift_expression */
    93,  /* (44) relational_expression ::= relational_expression PREFIX shift_expression */
    93,  /* (45) relational_expression ::= relational_expression SUFFIX shift_expression */
    93,  /* (46) relational_expression ::= relational_expression REGEXP shift_expression */
    94,  /* (47) shift_expression ::= shift_expression SHIFTL additive_expression */
    94,  /* (48) shift_expression ::= shift_expression SHIFTR additive_expression */
    94,  /* (49) shift_expression ::= shift_expression SHIFTRR additive_expression */
    95,  /* (50) additive_expression ::= additive_expression PLUS multiplicative_expression */
    95,  /* (51) additive_expression ::= additive_expression MINUS multiplicative_expression */
    96,  /* (52) multiplicative_expression ::= multiplicative_expression STAR unary_expression */
    96,  /* (53) multiplicative_expression ::= multiplicative_expression SLASH unary_expression */
    96,  /* (54) multiplicative_expression ::= multiplicative_expression MOD unary_expression */
    97,  /* (55) unary_expression ::= DELETE unary_expression */
    97,  /* (56) unary_expression ::= INCR unary_expression */
    97,  /* (57) unary_expression ::= DECR unary_expression */
    97,  /* (58) unary_expression ::= PLUS unary_expression */
    97,  /* (59) unary_expression ::= MINUS unary_expression */
    97,  /* (60) unary_expression ::= NOT unary_expression */
    97,  /* (61) unary_expression ::= BITWISE_NOT unary_expression */
    97,  /* (62) unary_expression ::= ADJUST unary_expression */
    97,  /* (63) unary_expression ::= EXACT unary_expression */
    97,  /* (64) unary_expression ::= PARTIAL unary_expression */
    97,  /* (65) unary_expression ::= UNSPLIT unary_expression */
    98,  /* (66) postfix_expression ::= lefthand_side_expression INCR */
    98,  /* (67) postfix_expression ::= lefthand_side_expression DECR */
    99,  /* (68) call_expression ::= member_expression arguments */
   104,  /* (69) array_literal ::= BRACKETL element_list BRACKETR */
   105,  /* (70) element_list ::= */
   105,  /* (71) element_list ::= assignment_expression */
   103,  /* (72) object_literal ::= BRACEL property_name_and_value_list BRACER */
   106,  /* (73) property_name_and_value_list ::= */
   107,  /* (74) property_name_and_value ::= property_name COLON assignment_expression */
   102,  /* (75) member_expression_part ::= BRACKETL expression BRACKETR */
   101,  /* (76) arguments ::= PARENL argument_list PARENR */
   109,  /* (77) argument_list ::= */
   109,  /* (78) argument_list ::= assignment_expression */
   109,  /* (79) argument_list ::= argument_list COMMA assignment_expression */
    80,  /* (80) output_columns ::= */
    80,  /* (81) output_columns ::= output_column */
    80,  /* (82) output_columns ::= output_columns COMMA */
    80,  /* (83) output_columns ::= output_columns COMMA output_column */
   110,  /* (84) output_column ::= STAR */
   110,  /* (85) output_column ::= NONEXISTENT_COLUMN */
   110,  /* (86) output_column ::= assignment_expression */
    81,  /* (87) adjuster ::= adjuster PLUS adjust_expression */
   111,  /* (88) adjust_expression ::= adjust_match_expression STAR DECIMAL */
   112,  /* (89) adjust_match_expression ::= IDENTIFIER MATCH STRING */
    77,  /* (90) input ::= query */
    77,  /* (91) input ::= expression */
    77,  /* (92) input ::= START_OUTPUT_COLUMNS output_columns */
    77,  /* (93) input ::= START_ADJUSTER adjuster */
    78,  /* (94) query ::= query_element */
    82,  /* (95) query_element ::= QSTRING */
    82,  /* (96) query_element ::= PARENL query PARENR */
    79,  /* (97) expression ::= assignment_expression */
    84,  /* (98) assignment_expression ::= conditional_expression */
    85,  /* (99) conditional_expression ::= logical_or_expression */
    87,  /* (100) logical_or_expression ::= logical_and_expression */
    88,  /* (101) logical_and_expression ::= bitwise_or_expression */
    89,  /* (102) bitwise_or_expression ::= bitwise_xor_expression */
    90,  /* (103) bitwise_xor_expression ::= bitwise_and_expression */
    91,  /* (104) bitwise_and_expression ::= equality_expression */
    92,  /* (105) equality_expression ::= relational_expression */
    93,  /* (106) relational_expression ::= shift_expression */
    94,  /* (107) shift_expression ::= additive_expression */
    95,  /* (108) additive_expression ::= multiplicative_expression */
    96,  /* (109) multiplicative_expression ::= unary_expression */
    97,  /* (110) unary_expression ::= postfix_expression */
    98,  /* (111) postfix_expression ::= lefthand_side_expression */
    86,  /* (112) lefthand_side_expression ::= call_expression */
    86,  /* (113) lefthand_side_expression ::= member_expression */
   100,  /* (114) member_expression ::= primary_expression */
   100,  /* (115) member_expression ::= member_expression member_expression_part */
    83,  /* (116) primary_expression ::= object_literal */
    83,  /* (117) primary_expression ::= PARENL expression PARENR */
    83,  /* (118) primary_expression ::= IDENTIFIER */
    83,  /* (119) primary_expression ::= array_literal */
    83,  /* (120) primary_expression ::= DECIMAL */
    83,  /* (121) primary_expression ::= HEX_INTEGER */
    83,  /* (122) primary_expression ::= STRING */
    83,  /* (123) primary_expression ::= BOOLEAN */
    83,  /* (124) primary_expression ::= NULL */
   105,  /* (125) element_list ::= element_list COMMA assignment_expression */
   106,  /* (126) property_name_and_value_list ::= property_name_and_value */
   106,  /* (127) property_name_and_value_list ::= property_name_and_value_list COMMA property_name_and_value */
   108,  /* (128) property_name ::= STRING */
   102,  /* (129) member_expression_part ::= DOT IDENTIFIER */
    81,  /* (130) adjuster ::= */
    81,  /* (131) adjuster ::= adjust_expression */
   111,  /* (132) adjust_expression ::= adjust_match_expression */
};

/* For rule J, yyRuleInfoNRhs[J] contains the negative of the number
** of symbols on the right-hand side of that rule. */
static const signed char yyRuleInfoNRhs[] = {
   -2,  /* (0) query ::= query query_element */
   -3,  /* (1) query ::= query LOGICAL_AND query_element */
   -3,  /* (2) query ::= query LOGICAL_AND_NOT query_element */
   -3,  /* (3) query ::= query LOGICAL_OR query_element */
   -3,  /* (4) query ::= query NEGATIVE query_element */
   -2,  /* (5) query_element ::= ADJUST query_element */
   -2,  /* (6) query_element ::= RELATIVE_OP query_element */
   -3,  /* (7) query_element ::= IDENTIFIER RELATIVE_OP query_element */
   -3,  /* (8) query_element ::= BRACEL expression BRACER */
   -2,  /* (9) query_element ::= EVAL primary_expression */
   -3,  /* (10) expression ::= expression COMMA assignment_expression */
   -3,  /* (11) assignment_expression ::= lefthand_side_expression ASSIGN assignment_expression */
   -3,  /* (12) assignment_expression ::= lefthand_side_expression STAR_ASSIGN assignment_expression */
   -3,  /* (13) assignment_expression ::= lefthand_side_expression SLASH_ASSIGN assignment_expression */
   -3,  /* (14) assignment_expression ::= lefthand_side_expression MOD_ASSIGN assignment_expression */
   -3,  /* (15) assignment_expression ::= lefthand_side_expression PLUS_ASSIGN assignment_expression */
   -3,  /* (16) assignment_expression ::= lefthand_side_expression MINUS_ASSIGN assignment_expression */
   -3,  /* (17) assignment_expression ::= lefthand_side_expression SHIFTL_ASSIGN assignment_expression */
   -3,  /* (18) assignment_expression ::= lefthand_side_expression SHIFTR_ASSIGN assignment_expression */
   -3,  /* (19) assignment_expression ::= lefthand_side_expression SHIFTRR_ASSIGN assignment_expression */
   -3,  /* (20) assignment_expression ::= lefthand_side_expression AND_ASSIGN assignment_expression */
   -3,  /* (21) assignment_expression ::= lefthand_side_expression XOR_ASSIGN assignment_expression */
   -3,  /* (22) assignment_expression ::= lefthand_side_expression OR_ASSIGN assignment_expression */
   -5,  /* (23) conditional_expression ::= logical_or_expression QUESTION assignment_expression COLON assignment_expression */
   -3,  /* (24) logical_or_expression ::= logical_or_expression LOGICAL_OR logical_and_expression */
   -3,  /* (25) logical_and_expression ::= logical_and_expression LOGICAL_AND bitwise_or_expression */
   -3,  /* (26) logical_and_expression ::= logical_and_expression LOGICAL_AND_NOT bitwise_or_expression */
   -3,  /* (27) bitwise_or_expression ::= bitwise_or_expression BITWISE_OR bitwise_xor_expression */
   -3,  /* (28) bitwise_xor_expression ::= bitwise_xor_expression BITWISE_XOR bitwise_and_expression */
   -3,  /* (29) bitwise_and_expression ::= bitwise_and_expression BITWISE_AND equality_expression */
   -3,  /* (30) equality_expression ::= equality_expression EQUAL relational_expression */
   -3,  /* (31) equality_expression ::= equality_expression NOT_EQUAL relational_expression */
   -3,  /* (32) relational_expression ::= relational_expression LESS shift_expression */
   -3,  /* (33) relational_expression ::= relational_expression GREATER shift_expression */
   -3,  /* (34) relational_expression ::= relational_expression LESS_EQUAL shift_expression */
   -3,  /* (35) relational_expression ::= relational_expression GREATER_EQUAL shift_expression */
   -3,  /* (36) relational_expression ::= relational_expression IN shift_expression */
   -3,  /* (37) relational_expression ::= relational_expression MATCH shift_expression */
   -3,  /* (38) relational_expression ::= relational_expression NEAR shift_expression */
   -3,  /* (39) relational_expression ::= relational_expression NEAR2 shift_expression */
   -3,  /* (40) relational_expression ::= relational_expression SIMILAR shift_expression */
   -3,  /* (41) relational_expression ::= relational_expression TERM_EXTRACT shift_expression */
   -3,  /* (42) relational_expression ::= relational_expression QUORUM shift_expression */
   -3,  /* (43) relational_expression ::= relational_expression LCP shift_expression */
   -3,  /* (44) relational_expression ::= relational_expression PREFIX shift_expression */
   -3,  /* (45) relational_expression ::= relational_expression SUFFIX shift_expression */
   -3,  /* (46) relational_expression ::= relational_expression REGEXP shift_expression */
   -3,  /* (47) shift_expression ::= shift_expression SHIFTL additive_expression */
   -3,  /* (48) shift_expression ::= shift_expression SHIFTR additive_expression */
   -3,  /* (49) shift_expression ::= shift_expression SHIFTRR additive_expression */
   -3,  /* (50) additive_expression ::= additive_expression PLUS multiplicative_expression */
   -3,  /* (51) additive_expression ::= additive_expression MINUS multiplicative_expression */
   -3,  /* (52) multiplicative_expression ::= multiplicative_expression STAR unary_expression */
   -3,  /* (53) multiplicative_expression ::= multiplicative_expression SLASH unary_expression */
   -3,  /* (54) multiplicative_expression ::= multiplicative_expression MOD unary_expression */
   -2,  /* (55) unary_expression ::= DELETE unary_expression */
   -2,  /* (56) unary_expression ::= INCR unary_expression */
   -2,  /* (57) unary_expression ::= DECR unary_expression */
   -2,  /* (58) unary_expression ::= PLUS unary_expression */
   -2,  /* (59) unary_expression ::= MINUS unary_expression */
   -2,  /* (60) unary_expression ::= NOT unary_expression */
   -2,  /* (61) unary_expression ::= BITWISE_NOT unary_expression */
   -2,  /* (62) unary_expression ::= ADJUST unary_expression */
   -2,  /* (63) unary_expression ::= EXACT unary_expression */
   -2,  /* (64) unary_expression ::= PARTIAL unary_expression */
   -2,  /* (65) unary_expression ::= UNSPLIT unary_expression */
   -2,  /* (66) postfix_expression ::= lefthand_side_expression INCR */
   -2,  /* (67) postfix_expression ::= lefthand_side_expression DECR */
   -2,  /* (68) call_expression ::= member_expression arguments */
   -3,  /* (69) array_literal ::= BRACKETL element_list BRACKETR */
    0,  /* (70) element_list ::= */
   -1,  /* (71) element_list ::= assignment_expression */
   -3,  /* (72) object_literal ::= BRACEL property_name_and_value_list BRACER */
    0,  /* (73) property_name_and_value_list ::= */
   -3,  /* (74) property_name_and_value ::= property_name COLON assignment_expression */
   -3,  /* (75) member_expression_part ::= BRACKETL expression BRACKETR */
   -3,  /* (76) arguments ::= PARENL argument_list PARENR */
    0,  /* (77) argument_list ::= */
   -1,  /* (78) argument_list ::= assignment_expression */
   -3,  /* (79) argument_list ::= argument_list COMMA assignment_expression */
    0,  /* (80) output_columns ::= */
   -1,  /* (81) output_columns ::= output_column */
   -2,  /* (82) output_columns ::= output_columns COMMA */
   -3,  /* (83) output_columns ::= output_columns COMMA output_column */
   -1,  /* (84) output_column ::= STAR */
   -1,  /* (85) output_column ::= NONEXISTENT_COLUMN */
   -1,  /* (86) output_column ::= assignment_expression */
   -3,  /* (87) adjuster ::= adjuster PLUS adjust_expression */
   -3,  /* (88) adjust_expression ::= adjust_match_expression STAR DECIMAL */
   -3,  /* (89) adjust_match_expression ::= IDENTIFIER MATCH STRING */
   -1,  /* (90) input ::= query */
   -1,  /* (91) input ::= expression */
   -2,  /* (92) input ::= START_OUTPUT_COLUMNS output_columns */
   -2,  /* (93) input ::= START_ADJUSTER adjuster */
   -1,  /* (94) query ::= query_element */
   -1,  /* (95) query_element ::= QSTRING */
   -3,  /* (96) query_element ::= PARENL query PARENR */
   -1,  /* (97) expression ::= assignment_expression */
   -1,  /* (98) assignment_expression ::= conditional_expression */
   -1,  /* (99) conditional_expression ::= logical_or_expression */
   -1,  /* (100) logical_or_expression ::= logical_and_expression */
   -1,  /* (101) logical_and_expression ::= bitwise_or_expression */
   -1,  /* (102) bitwise_or_expression ::= bitwise_xor_expression */
   -1,  /* (103) bitwise_xor_expression ::= bitwise_and_expression */
   -1,  /* (104) bitwise_and_expression ::= equality_expression */
   -1,  /* (105) equality_expression ::= relational_expression */
   -1,  /* (106) relational_expression ::= shift_expression */
   -1,  /* (107) shift_expression ::= additive_expression */
   -1,  /* (108) additive_expression ::= multiplicative_expression */
   -1,  /* (109) multiplicative_expression ::= unary_expression */
   -1,  /* (110) unary_expression ::= postfix_expression */
   -1,  /* (111) postfix_expression ::= lefthand_side_expression */
   -1,  /* (112) lefthand_side_expression ::= call_expression */
   -1,  /* (113) lefthand_side_expression ::= member_expression */
   -1,  /* (114) member_expression ::= primary_expression */
   -2,  /* (115) member_expression ::= member_expression member_expression_part */
   -1,  /* (116) primary_expression ::= object_literal */
   -3,  /* (117) primary_expression ::= PARENL expression PARENR */
   -1,  /* (118) primary_expression ::= IDENTIFIER */
   -1,  /* (119) primary_expression ::= array_literal */
   -1,  /* (120) primary_expression ::= DECIMAL */
   -1,  /* (121) primary_expression ::= HEX_INTEGER */
   -1,  /* (122) primary_expression ::= STRING */
   -1,  /* (123) primary_expression ::= BOOLEAN */
   -1,  /* (124) primary_expression ::= NULL */
   -3,  /* (125) element_list ::= element_list COMMA assignment_expression */
   -1,  /* (126) property_name_and_value_list ::= property_name_and_value */
   -3,  /* (127) property_name_and_value_list ::= property_name_and_value_list COMMA property_name_and_value */
   -1,  /* (128) property_name ::= STRING */
   -2,  /* (129) member_expression_part ::= DOT IDENTIFIER */
    0,  /* (130) adjuster ::= */
   -1,  /* (131) adjuster ::= adjust_expression */
   -1,  /* (132) adjust_expression ::= adjust_match_expression */
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
**
** The yyLookahead and yyLookaheadToken parameters provide reduce actions
** access to the lookahead token (if any).  The yyLookahead will be YYNOCODE
** if the lookahead token has already been consumed.  As this procedure is
** only called from one place, optimizing compilers will in-line it, which
** means that the extra parameters have no performance impact.
*/
static YYACTIONTYPE yy_reduce(
  yyParser *yypParser,         /* The parser */
  unsigned int yyruleno,       /* Number of the rule by which to reduce */
  int yyLookahead,             /* Lookahead token, or YYNOCODE if none */
  grn_expr_parserTOKENTYPE yyLookaheadToken  /* Value of the lookahead token */
  grn_expr_parserCTX_PDECL                   /* %extra_context */
){
  int yygoto;                     /* The next state */
  YYACTIONTYPE yyact;             /* The next action */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  grn_expr_parserARG_FETCH
  (void)yyLookahead;
  (void)yyLookaheadToken;
  yymsp = yypParser->yytos;
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    yysize = yyRuleInfoNRhs[yyruleno];
    if( yysize ){
      fprintf(yyTraceFILE, "%sReduce %d [%s], go to state %d.\n",
        yyTracePrompt,
        yyruleno, yyRuleName[yyruleno], yymsp[yysize].stateno);
    }else{
      fprintf(yyTraceFILE, "%sReduce %d [%s].\n",
        yyTracePrompt, yyruleno, yyRuleName[yyruleno]);
    }
  }
#endif /* NDEBUG */

  /* Check that the stack is large enough to grow by a single entry
  ** if the RHS of the rule is empty.  This ensures that there is room
  ** enough on the stack to push the LHS value */
  if( yyRuleInfoNRhs[yyruleno]==0 ){
#ifdef YYTRACKMAXSTACKDEPTH
    if( (int)(yypParser->yytos - yypParser->yystack)>yypParser->yyhwm ){
      yypParser->yyhwm++;
      assert( yypParser->yyhwm == (int)(yypParser->yytos - yypParser->yystack));
    }
#endif
#if YYSTACKDEPTH>0 
    if( yypParser->yytos>=yypParser->yystackEnd ){
      yyStackOverflow(yypParser);
      /* The call to yyStackOverflow() above pops the stack until it is
      ** empty, causing the main parser loop to exit.  So the return value
      ** is never used and does not matter. */
      return 0;
    }
#else
    if( yypParser->yytos>=&yypParser->yystack[yypParser->yystksz-1] ){
      if( yyGrowStack(yypParser) ){
        yyStackOverflow(yypParser);
        /* The call to yyStackOverflow() above pops the stack until it is
        ** empty, causing the main parser loop to exit.  So the return value
        ** is never used and does not matter. */
        return 0;
      }
      yymsp = yypParser->yytos;
    }
#endif
  }

  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
/********** Begin reduce actions **********************************************/
        YYMINORTYPE yylhsminor;
      case 0: /* query ::= query query_element */
#line 55 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, grn_int32_value_at(&efsi->op_stack, -1), 2);
}
#line 1731 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 1: /* query ::= query LOGICAL_AND query_element */
      case 25: /* logical_and_expression ::= logical_and_expression LOGICAL_AND bitwise_or_expression */ yytestcase(yyruleno==25);
#line 58 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND, 2);
}
#line 1739 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 2: /* query ::= query LOGICAL_AND_NOT query_element */
      case 26: /* logical_and_expression ::= logical_and_expression LOGICAL_AND_NOT bitwise_or_expression */ yytestcase(yyruleno==26);
#line 61 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND_NOT, 2);
}
#line 1747 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 3: /* query ::= query LOGICAL_OR query_element */
      case 24: /* logical_or_expression ::= logical_or_expression LOGICAL_OR logical_and_expression */ yytestcase(yyruleno==24);
#line 64 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_OR, 2);
}
#line 1755 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 4: /* query ::= query NEGATIVE query_element */
#line 67 "../../groonga/lib/grn_ecmascript.lemon"
{
  int weight;
  GRN_INT32_POP(&efsi->weight_stack, weight);
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_ADJUST, 2);
}
#line 1764 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 5: /* query_element ::= ADJUST query_element */
#line 76 "../../groonga/lib/grn_ecmascript.lemon"
{
  int weight;
  GRN_INT32_POP(&efsi->weight_stack, weight);
}
#line 1772 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 6: /* query_element ::= RELATIVE_OP query_element */
#line 80 "../../groonga/lib/grn_ecmascript.lemon"
{
  int mode;
  GRN_INT32_POP(&efsi->mode_stack, mode);
}
#line 1780 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 7: /* query_element ::= IDENTIFIER RELATIVE_OP query_element */
#line 84 "../../groonga/lib/grn_ecmascript.lemon"
{
  int mode;
  grn_obj *c;
  GRN_PTR_POP(&efsi->column_stack, c);
  GRN_INT32_POP(&efsi->mode_stack, mode);
  switch (mode) {
  case GRN_OP_NEAR :
  case GRN_OP_NEAR2 :
    {
      int max_interval;
      GRN_INT32_POP(&efsi->max_interval_stack, max_interval);
    }
    break;
  case GRN_OP_SIMILAR :
    {
      int similarity_threshold;
      GRN_INT32_POP(&efsi->similarity_threshold_stack, similarity_threshold);
    }
    break;
  case GRN_OP_QUORUM :
    {
      int quorum_threshold;
      GRN_INT32_POP(&efsi->quorum_threshold_stack, quorum_threshold);
    }
    break;
  default :
    break;
  }
}
#line 1813 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 8: /* query_element ::= BRACEL expression BRACER */
      case 9: /* query_element ::= EVAL primary_expression */ yytestcase(yyruleno==9);
#line 113 "../../groonga/lib/grn_ecmascript.lemon"
{
  efsi->flags = efsi->default_flags;
}
#line 1821 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 10: /* expression ::= expression COMMA assignment_expression */
#line 121 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_COMMA, 2);
}
#line 1828 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 11: /* assignment_expression ::= lefthand_side_expression ASSIGN assignment_expression */
#line 126 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_ASSIGN, 2);
}
#line 1835 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 12: /* assignment_expression ::= lefthand_side_expression STAR_ASSIGN assignment_expression */
#line 129 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_STAR_ASSIGN, 2);
}
#line 1842 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 13: /* assignment_expression ::= lefthand_side_expression SLASH_ASSIGN assignment_expression */
#line 132 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SLASH_ASSIGN, 2);
}
#line 1849 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 14: /* assignment_expression ::= lefthand_side_expression MOD_ASSIGN assignment_expression */
#line 135 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MOD_ASSIGN, 2);
}
#line 1856 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 15: /* assignment_expression ::= lefthand_side_expression PLUS_ASSIGN assignment_expression */
#line 138 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PLUS_ASSIGN, 2);
}
#line 1863 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 16: /* assignment_expression ::= lefthand_side_expression MINUS_ASSIGN assignment_expression */
#line 141 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MINUS_ASSIGN, 2);
}
#line 1870 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 17: /* assignment_expression ::= lefthand_side_expression SHIFTL_ASSIGN assignment_expression */
#line 144 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTL_ASSIGN, 2);
}
#line 1877 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 18: /* assignment_expression ::= lefthand_side_expression SHIFTR_ASSIGN assignment_expression */
#line 147 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTR_ASSIGN, 2);
}
#line 1884 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 19: /* assignment_expression ::= lefthand_side_expression SHIFTRR_ASSIGN assignment_expression */
#line 150 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTRR_ASSIGN, 2);
}
#line 1891 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 20: /* assignment_expression ::= lefthand_side_expression AND_ASSIGN assignment_expression */
#line 153 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND_ASSIGN, 2);
}
#line 1898 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 21: /* assignment_expression ::= lefthand_side_expression XOR_ASSIGN assignment_expression */
#line 156 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_XOR_ASSIGN, 2);
}
#line 1905 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 22: /* assignment_expression ::= lefthand_side_expression OR_ASSIGN assignment_expression */
#line 159 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_OR_ASSIGN, 2);
}
#line 1912 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 23: /* conditional_expression ::= logical_or_expression QUESTION assignment_expression COLON assignment_expression */
#line 164 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr *e = (grn_expr *)efsi->e;
  e->codes[yymsp[-3].minor.yy0].nargs = yymsp[-1].minor.yy0 - yymsp[-3].minor.yy0;
  e->codes[yymsp[-1].minor.yy0].nargs = e->codes_curr - yymsp[-1].minor.yy0 - 1;
}
#line 1921 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 27: /* bitwise_or_expression ::= bitwise_or_expression BITWISE_OR bitwise_xor_expression */
#line 184 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_OR, 2);
}
#line 1928 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 28: /* bitwise_xor_expression ::= bitwise_xor_expression BITWISE_XOR bitwise_and_expression */
#line 189 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_XOR, 2);
}
#line 1935 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 29: /* bitwise_and_expression ::= bitwise_and_expression BITWISE_AND equality_expression */
#line 194 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_AND, 2);
}
#line 1942 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 30: /* equality_expression ::= equality_expression EQUAL relational_expression */
#line 199 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_EQUAL, 2);
}
#line 1949 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 31: /* equality_expression ::= equality_expression NOT_EQUAL relational_expression */
#line 202 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NOT_EQUAL, 2);
}
#line 1956 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 32: /* relational_expression ::= relational_expression LESS shift_expression */
#line 207 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_LESS, 2);
}
#line 1963 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 33: /* relational_expression ::= relational_expression GREATER shift_expression */
#line 210 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_GREATER, 2);
}
#line 1970 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 34: /* relational_expression ::= relational_expression LESS_EQUAL shift_expression */
#line 213 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_LESS_EQUAL, 2);
}
#line 1977 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 35: /* relational_expression ::= relational_expression GREATER_EQUAL shift_expression */
#line 216 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_GREATER_EQUAL, 2);
}
#line 1984 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 36: /* relational_expression ::= relational_expression IN shift_expression */
#line 219 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_IN, 2);
}
#line 1991 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 37: /* relational_expression ::= relational_expression MATCH shift_expression */
      case 89: /* adjust_match_expression ::= IDENTIFIER MATCH STRING */ yytestcase(yyruleno==89);
#line 222 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MATCH, 2);
}
#line 1999 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 38: /* relational_expression ::= relational_expression NEAR shift_expression */
#line 225 "../../groonga/lib/grn_ecmascript.lemon"
{
  {
    int max_interval;
    GRN_INT32_POP(&efsi->max_interval_stack, max_interval);
    grn_expr_append_const_int(efsi->ctx, efsi->e, max_interval,
                              GRN_OP_PUSH, 1);
  }
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NEAR, 3);
}
#line 2012 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 39: /* relational_expression ::= relational_expression NEAR2 shift_expression */
#line 234 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NEAR2, 2);
}
#line 2019 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 40: /* relational_expression ::= relational_expression SIMILAR shift_expression */
#line 237 "../../groonga/lib/grn_ecmascript.lemon"
{
  {
    int similarity_threshold;
    GRN_INT32_POP(&efsi->similarity_threshold_stack, similarity_threshold);
    grn_expr_append_const_int(efsi->ctx, efsi->e, similarity_threshold,
                              GRN_OP_PUSH, 1);
  }
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SIMILAR, 3);
}
#line 2032 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 41: /* relational_expression ::= relational_expression TERM_EXTRACT shift_expression */
#line 246 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_TERM_EXTRACT, 2);
}
#line 2039 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 42: /* relational_expression ::= relational_expression QUORUM shift_expression */
#line 249 "../../groonga/lib/grn_ecmascript.lemon"
{
  {
    int quorum_threshold;
    GRN_INT32_POP(&efsi->quorum_threshold_stack, quorum_threshold);
    grn_expr_append_const_int(efsi->ctx, efsi->e, quorum_threshold,
                              GRN_OP_PUSH, 1);
  }
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_QUORUM, 3);
}
#line 2052 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 43: /* relational_expression ::= relational_expression LCP shift_expression */
#line 258 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_LCP, 2);
}
#line 2059 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 44: /* relational_expression ::= relational_expression PREFIX shift_expression */
#line 261 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PREFIX, 2);
}
#line 2066 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 45: /* relational_expression ::= relational_expression SUFFIX shift_expression */
#line 264 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SUFFIX, 2);
}
#line 2073 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 46: /* relational_expression ::= relational_expression REGEXP shift_expression */
#line 267 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_REGEXP, 2);
}
#line 2080 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 47: /* shift_expression ::= shift_expression SHIFTL additive_expression */
#line 272 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTL, 2);
}
#line 2087 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 48: /* shift_expression ::= shift_expression SHIFTR additive_expression */
#line 275 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTR, 2);
}
#line 2094 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 49: /* shift_expression ::= shift_expression SHIFTRR additive_expression */
#line 278 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTRR, 2);
}
#line 2101 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 50: /* additive_expression ::= additive_expression PLUS multiplicative_expression */
      case 87: /* adjuster ::= adjuster PLUS adjust_expression */ yytestcase(yyruleno==87);
#line 283 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PLUS, 2);
}
#line 2109 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 51: /* additive_expression ::= additive_expression MINUS multiplicative_expression */
#line 286 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MINUS, 2);
}
#line 2116 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 52: /* multiplicative_expression ::= multiplicative_expression STAR unary_expression */
      case 88: /* adjust_expression ::= adjust_match_expression STAR DECIMAL */ yytestcase(yyruleno==88);
#line 291 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_STAR, 2);
}
#line 2124 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 53: /* multiplicative_expression ::= multiplicative_expression SLASH unary_expression */
#line 294 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SLASH, 2);
}
#line 2131 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 54: /* multiplicative_expression ::= multiplicative_expression MOD unary_expression */
#line 297 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MOD, 2);
}
#line 2138 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 55: /* unary_expression ::= DELETE unary_expression */
#line 302 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_DELETE, 1);
}
#line 2145 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 56: /* unary_expression ::= INCR unary_expression */
#line 305 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_ctx *ctx = efsi->ctx;
  grn_expr *e = (grn_expr *)(efsi->e);
  grn_expr_dfi *dfi_;
  unsigned int const_p;

  dfi_ = grn_expr_dfi_pop(e);
  const_p = CONSTP(dfi_->code->value);
  grn_expr_dfi_put(ctx, e, dfi_->type, dfi_->domain, dfi_->code);
  if (const_p) {
    ERR(GRN_SYNTAX_ERROR,
        "constant can't be incremented: <%.*s>",
        (int)(efsi->str_end - efsi->str), efsi->str);
  } else {
    grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_INCR, 1);
  }
}
#line 2166 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 57: /* unary_expression ::= DECR unary_expression */
#line 322 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_ctx *ctx = efsi->ctx;
  grn_expr *e = (grn_expr *)(efsi->e);
  grn_expr_dfi *dfi_;
  unsigned int const_p;

  dfi_ = grn_expr_dfi_pop(e);
  const_p = CONSTP(dfi_->code->value);
  grn_expr_dfi_put(ctx, e, dfi_->type, dfi_->domain, dfi_->code);
  if (const_p) {
    ERR(GRN_SYNTAX_ERROR,
        "constant can't be decremented: <%.*s>",
        (int)(efsi->str_end - efsi->str), efsi->str);
  } else {
    grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_DECR, 1);
  }
}
#line 2187 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 58: /* unary_expression ::= PLUS unary_expression */
#line 339 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PLUS, 1);
}
#line 2194 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 59: /* unary_expression ::= MINUS unary_expression */
#line 342 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MINUS, 1);
}
#line 2201 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 60: /* unary_expression ::= NOT unary_expression */
#line 345 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NOT, 1);
}
#line 2208 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 61: /* unary_expression ::= BITWISE_NOT unary_expression */
#line 348 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_NOT, 1);
}
#line 2215 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 62: /* unary_expression ::= ADJUST unary_expression */
#line 351 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_ADJUST, 1);
}
#line 2222 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 63: /* unary_expression ::= EXACT unary_expression */
#line 354 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_EXACT, 1);
}
#line 2229 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 64: /* unary_expression ::= PARTIAL unary_expression */
#line 357 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PARTIAL, 1);
}
#line 2236 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 65: /* unary_expression ::= UNSPLIT unary_expression */
#line 360 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_UNSPLIT, 1);
}
#line 2243 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 66: /* postfix_expression ::= lefthand_side_expression INCR */
#line 365 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_ctx *ctx = efsi->ctx;
  grn_expr *e = (grn_expr *)(efsi->e);
  grn_expr_dfi *dfi_;
  unsigned int const_p;

  dfi_ = grn_expr_dfi_pop(e);
  const_p = CONSTP(dfi_->code->value);
  grn_expr_dfi_put(ctx, e, dfi_->type, dfi_->domain, dfi_->code);
  if (const_p) {
    ERR(GRN_SYNTAX_ERROR,
        "constant can't be incremented: <%.*s>",
        (int)(efsi->str_end - efsi->str), efsi->str);
  } else {
    grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_INCR_POST, 1);
  }
}
#line 2264 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 67: /* postfix_expression ::= lefthand_side_expression DECR */
#line 382 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_ctx *ctx = efsi->ctx;
  grn_expr *e = (grn_expr *)(efsi->e);
  grn_expr_dfi *dfi_;
  unsigned int const_p;

  dfi_ = grn_expr_dfi_pop(e);
  const_p = CONSTP(dfi_->code->value);
  grn_expr_dfi_put(ctx, e, dfi_->type, dfi_->domain, dfi_->code);
  if (const_p) {
    ERR(GRN_SYNTAX_ERROR,
        "constant can't be decremented: <%.*s>",
        (int)(efsi->str_end - efsi->str), efsi->str);
  } else {
    grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_DECR_POST, 1);
  }
}
#line 2285 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 68: /* call_expression ::= member_expression arguments */
#line 403 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_CALL, yymsp[0].minor.yy0);
}
#line 2292 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 69: /* array_literal ::= BRACKETL element_list BRACKETR */
#line 420 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_ctx *ctx = efsi->ctx;
  if (efsi->array_literal) {
    grn_expr_take_obj(ctx, efsi->e, efsi->array_literal);
    grn_expr_append_obj(ctx, efsi->e, efsi->array_literal,
                        GRN_OP_PUSH, 1);
    efsi->array_literal = NULL;
  }
}
#line 2305 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 70: /* element_list ::= */
#line 430 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_ctx *ctx = efsi->ctx;

  efsi->array_literal = grn_obj_open(ctx, GRN_VECTOR, 0, GRN_ID_NIL);
  if (!efsi->array_literal) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "couldn't create vector for parsing array literal: <%.*s>",
        (int)(efsi->str_end - efsi->str), efsi->str);
  }
}
#line 2319 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 71: /* element_list ::= assignment_expression */
#line 440 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_ctx *ctx = efsi->ctx;
  grn_expr *e = (grn_expr *)(efsi->e);
  grn_expr_code *code = &(e->codes[e->codes_curr - 1]);
  if (code->op != GRN_OP_PUSH) {
    if (!efsi->array_literal) {
      efsi->array_literal = grn_obj_open(ctx, GRN_VECTOR, 0, GRN_ID_NIL);
    }
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
        "complex expression in array literal isn't supported yet: <%.*s>",
        (int)(efsi->str_end - efsi->str), efsi->str);
  }
  if (ctx->rc == GRN_SUCCESS) {
    grn_obj *element = code->value;
    if (!efsi->array_literal) {
      efsi->array_literal = grn_obj_open(ctx,
                                         GRN_VECTOR,
                                         0,
                                         element->header.domain);
      if (!efsi->array_literal) {
        ERR(GRN_NO_MEMORY_AVAILABLE,
            "couldn't create vector for parsing array literal: <%.*s>",
            (int)(efsi->str_end - efsi->str), efsi->str);
      }
    }
    if (efsi->array_literal) {
      grn_vector_add_element(ctx,
                             efsi->array_literal,
                             GRN_TEXT_VALUE(element),
                             GRN_TEXT_LEN(element),
                             0,
                             element->header.domain);
      if (ctx->rc == GRN_SUCCESS) {
        grn_expr_dfi_pop(e);
      e->codes_curr -= 1;
      }
    }
  }
}
#line 2362 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 72: /* object_literal ::= BRACEL property_name_and_value_list BRACER */
#line 481 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_ctx *ctx = efsi->ctx;
  grn_expr_take_obj(ctx, efsi->e, (grn_obj *)(efsi->object_literal));
  grn_expr_append_obj(ctx, efsi->e, (grn_obj *)(efsi->object_literal),
                      GRN_OP_PUSH, 1);
  efsi->object_literal = NULL;
}
#line 2373 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 73: /* property_name_and_value_list ::= */
#line 489 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_ctx *ctx = efsi->ctx;

  efsi->object_literal =
    grn_hash_create(ctx, NULL, GRN_TABLE_MAX_KEY_SIZE, sizeof(grn_obj),
                    GRN_OBJ_KEY_VAR_SIZE|GRN_OBJ_TEMPORARY|GRN_HASH_TINY);
  if (!efsi->object_literal) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "couldn't create hash table for parsing object literal: <%.*s>",
        (int)(efsi->str_end - efsi->str), efsi->str);
  }
}
#line 2389 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 74: /* property_name_and_value ::= property_name COLON assignment_expression */
#line 504 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_ctx *ctx = efsi->ctx;
  grn_expr *e = (grn_expr *)(efsi->e);
  grn_obj *property = e->codes[e->codes_curr - 3].value;
  grn_obj *value = e->codes[e->codes_curr - 1].value;

  if (!efsi->object_literal) {
     efsi->object_literal =
       grn_hash_create(ctx, NULL, GRN_TABLE_MAX_KEY_SIZE, sizeof(grn_obj),
                       GRN_OBJ_KEY_VAR_SIZE|GRN_OBJ_TEMPORARY|GRN_HASH_TINY);
  }

  if (!efsi->object_literal) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "couldn't create hash table for parsing object literal: <%.*s>",
        (int)(efsi->str_end - efsi->str), efsi->str);
  } else {
    grn_obj *buf;
    int added;
    if (grn_hash_add(ctx, (grn_hash *)efsi->object_literal,
                     GRN_TEXT_VALUE(property), GRN_TEXT_LEN(property),
                     (void **)&buf, &added)) {
      if (added) {
        GRN_OBJ_INIT(buf, value->header.type, 0, value->header.domain);
        GRN_TEXT_PUT(ctx, buf, GRN_TEXT_VALUE(value), GRN_TEXT_LEN(value));
        grn_expr_dfi_pop(e);
        e->codes_curr -= 3;
      } else {
        ERR(GRN_INVALID_ARGUMENT,
            "duplicated property name: <%.*s>",
            (int)GRN_TEXT_LEN(property),
            GRN_TEXT_VALUE(property));
      }
    } else {
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "failed to add a property to object literal: <%.*s>",
          (int)GRN_TEXT_LEN(property),
          GRN_TEXT_VALUE(property));
    }
  }
}
#line 2434 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 75: /* member_expression_part ::= BRACKETL expression BRACKETR */
#line 548 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_GET_MEMBER, 2);
}
#line 2441 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 76: /* arguments ::= PARENL argument_list PARENR */
#line 553 "../../groonga/lib/grn_ecmascript.lemon"
{ yymsp[-2].minor.yy0 = yymsp[-1].minor.yy0; }
#line 2446 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 77: /* argument_list ::= */
#line 554 "../../groonga/lib/grn_ecmascript.lemon"
{ yymsp[1].minor.yy0 = 0; }
#line 2451 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 78: /* argument_list ::= assignment_expression */
#line 555 "../../groonga/lib/grn_ecmascript.lemon"
{ yymsp[0].minor.yy0 = 1; }
#line 2456 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 79: /* argument_list ::= argument_list COMMA assignment_expression */
#line 556 "../../groonga/lib/grn_ecmascript.lemon"
{ yylhsminor.yy0 = yymsp[-2].minor.yy0 + 1; }
#line 2461 "../../groonga/lib/grn_ecmascript.c"
  yymsp[-2].minor.yy0 = yylhsminor.yy0;
        break;
      case 80: /* output_columns ::= */
#line 558 "../../groonga/lib/grn_ecmascript.lemon"
{
  yymsp[1].minor.yy0 = 0;
}
#line 2469 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 81: /* output_columns ::= output_column */
#line 561 "../../groonga/lib/grn_ecmascript.lemon"
{
  yylhsminor.yy0 = yymsp[0].minor.yy0;
}
#line 2476 "../../groonga/lib/grn_ecmascript.c"
  yymsp[0].minor.yy0 = yylhsminor.yy0;
        break;
      case 82: /* output_columns ::= output_columns COMMA */
#line 566 "../../groonga/lib/grn_ecmascript.lemon"
{
  yylhsminor.yy0 = yymsp[-1].minor.yy0;
}
#line 2484 "../../groonga/lib/grn_ecmascript.c"
  yymsp[-1].minor.yy0 = yylhsminor.yy0;
        break;
      case 83: /* output_columns ::= output_columns COMMA output_column */
#line 571 "../../groonga/lib/grn_ecmascript.lemon"
{
  if (yymsp[-2].minor.yy0 == 0) {
    yylhsminor.yy0 = yymsp[0].minor.yy0;
  } else if (yymsp[0].minor.yy0 == 0) {
    yylhsminor.yy0 = yymsp[-2].minor.yy0;
  } else {
    if (yymsp[0].minor.yy0 == 1) {
      grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_COMMA, 2);
    }
    yylhsminor.yy0 = 1;
  }
}
#line 2501 "../../groonga/lib/grn_ecmascript.c"
  yymsp[-2].minor.yy0 = yylhsminor.yy0;
        break;
      case 84: /* output_column ::= STAR */
#line 584 "../../groonga/lib/grn_ecmascript.lemon"
{
  grn_ctx *ctx = efsi->ctx;
  grn_obj *expr = efsi->e;
  grn_obj *variable = grn_expr_get_var_by_offset(ctx, expr, 0);
  if (variable) {
    grn_id table_id = GRN_OBJ_GET_DOMAIN(variable);
    grn_obj *table = grn_ctx_at(ctx, table_id);
    grn_obj columns_buffer;
    int n_columns;
    grn_obj **columns;

    GRN_PTR_INIT(&columns_buffer, GRN_OBJ_VECTOR, GRN_ID_NIL);
    grn_obj_columns(ctx, table, "*", strlen("*"), &columns_buffer);
    n_columns = GRN_BULK_VSIZE(&columns_buffer) / sizeof(grn_obj *);
    columns = (grn_obj **)GRN_BULK_HEAD(&columns_buffer);

    if (n_columns == 0) {
      /* do nothing */
    } else if (n_columns == 1) {
      grn_obj *column = columns[0];
      grn_expr_append_const(ctx, expr, column, GRN_OP_GET_VALUE, 1);
      if (column->header.type == GRN_ACCESSOR) {
        grn_expr_take_obj(ctx, expr, column);
      }
    } else {
      grn_expr *e = (grn_expr *)expr;
      grn_bool have_column;
      int i;

      have_column = (e->codes_curr > 0);
      for (i = 0; i < n_columns; i++) {
        grn_obj *column = columns[i];
        grn_expr_append_const(ctx, expr, column, GRN_OP_GET_VALUE, 1);
        if (have_column || i > 0) {
          grn_expr_append_op(ctx, expr, GRN_OP_COMMA, 2);
        }
        if (column->header.type == GRN_ACCESSOR) {
          grn_expr_take_obj(ctx, expr, column);
        }
      }
    }

    GRN_OBJ_FIN(ctx, &columns_buffer);

    yymsp[0].minor.yy0 = n_columns;
  } else {
    /* TODO: report error */
    yymsp[0].minor.yy0 = 0;
  }
}
#line 2556 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 85: /* output_column ::= NONEXISTENT_COLUMN */
#line 634 "../../groonga/lib/grn_ecmascript.lemon"
{
  yymsp[0].minor.yy0 = 0;
}
#line 2563 "../../groonga/lib/grn_ecmascript.c"
        break;
      case 86: /* output_column ::= assignment_expression */
#line 637 "../../groonga/lib/grn_ecmascript.lemon"
{
  yymsp[0].minor.yy0 = 1;
}
#line 2570 "../../groonga/lib/grn_ecmascript.c"
        break;
      default:
      /* (90) input ::= query */ yytestcase(yyruleno==90);
      /* (91) input ::= expression */ yytestcase(yyruleno==91);
      /* (92) input ::= START_OUTPUT_COLUMNS output_columns */ yytestcase(yyruleno==92);
      /* (93) input ::= START_ADJUSTER adjuster */ yytestcase(yyruleno==93);
      /* (94) query ::= query_element (OPTIMIZED OUT) */ assert(yyruleno!=94);
      /* (95) query_element ::= QSTRING */ yytestcase(yyruleno==95);
      /* (96) query_element ::= PARENL query PARENR */ yytestcase(yyruleno==96);
      /* (97) expression ::= assignment_expression (OPTIMIZED OUT) */ assert(yyruleno!=97);
      /* (98) assignment_expression ::= conditional_expression (OPTIMIZED OUT) */ assert(yyruleno!=98);
      /* (99) conditional_expression ::= logical_or_expression */ yytestcase(yyruleno==99);
      /* (100) logical_or_expression ::= logical_and_expression */ yytestcase(yyruleno==100);
      /* (101) logical_and_expression ::= bitwise_or_expression */ yytestcase(yyruleno==101);
      /* (102) bitwise_or_expression ::= bitwise_xor_expression */ yytestcase(yyruleno==102);
      /* (103) bitwise_xor_expression ::= bitwise_and_expression */ yytestcase(yyruleno==103);
      /* (104) bitwise_and_expression ::= equality_expression */ yytestcase(yyruleno==104);
      /* (105) equality_expression ::= relational_expression */ yytestcase(yyruleno==105);
      /* (106) relational_expression ::= shift_expression */ yytestcase(yyruleno==106);
      /* (107) shift_expression ::= additive_expression */ yytestcase(yyruleno==107);
      /* (108) additive_expression ::= multiplicative_expression */ yytestcase(yyruleno==108);
      /* (109) multiplicative_expression ::= unary_expression (OPTIMIZED OUT) */ assert(yyruleno!=109);
      /* (110) unary_expression ::= postfix_expression (OPTIMIZED OUT) */ assert(yyruleno!=110);
      /* (111) postfix_expression ::= lefthand_side_expression */ yytestcase(yyruleno==111);
      /* (112) lefthand_side_expression ::= call_expression (OPTIMIZED OUT) */ assert(yyruleno!=112);
      /* (113) lefthand_side_expression ::= member_expression */ yytestcase(yyruleno==113);
      /* (114) member_expression ::= primary_expression (OPTIMIZED OUT) */ assert(yyruleno!=114);
      /* (115) member_expression ::= member_expression member_expression_part */ yytestcase(yyruleno==115);
      /* (116) primary_expression ::= object_literal (OPTIMIZED OUT) */ assert(yyruleno!=116);
      /* (117) primary_expression ::= PARENL expression PARENR */ yytestcase(yyruleno==117);
      /* (118) primary_expression ::= IDENTIFIER */ yytestcase(yyruleno==118);
      /* (119) primary_expression ::= array_literal (OPTIMIZED OUT) */ assert(yyruleno!=119);
      /* (120) primary_expression ::= DECIMAL */ yytestcase(yyruleno==120);
      /* (121) primary_expression ::= HEX_INTEGER */ yytestcase(yyruleno==121);
      /* (122) primary_expression ::= STRING */ yytestcase(yyruleno==122);
      /* (123) primary_expression ::= BOOLEAN */ yytestcase(yyruleno==123);
      /* (124) primary_expression ::= NULL */ yytestcase(yyruleno==124);
      /* (125) element_list ::= element_list COMMA assignment_expression */ yytestcase(yyruleno==125);
      /* (126) property_name_and_value_list ::= property_name_and_value (OPTIMIZED OUT) */ assert(yyruleno!=126);
      /* (127) property_name_and_value_list ::= property_name_and_value_list COMMA property_name_and_value */ yytestcase(yyruleno==127);
      /* (128) property_name ::= STRING */ yytestcase(yyruleno==128);
      /* (129) member_expression_part ::= DOT IDENTIFIER */ yytestcase(yyruleno==129);
      /* (130) adjuster ::= */ yytestcase(yyruleno==130);
      /* (131) adjuster ::= adjust_expression (OPTIMIZED OUT) */ assert(yyruleno!=131);
      /* (132) adjust_expression ::= adjust_match_expression */ yytestcase(yyruleno==132);
        break;
/********** End reduce actions ************************************************/
  };
  assert( yyruleno<sizeof(yyRuleInfoLhs)/sizeof(yyRuleInfoLhs[0]) );
  yygoto = yyRuleInfoLhs[yyruleno];
  yysize = yyRuleInfoNRhs[yyruleno];
  yyact = yy_find_reduce_action(yymsp[yysize].stateno,(YYCODETYPE)yygoto);

  /* There are no SHIFTREDUCE actions on nonterminals because the table
  ** generator has simplified them to pure REDUCE actions. */
  assert( !(yyact>YY_MAX_SHIFT && yyact<=YY_MAX_SHIFTREDUCE) );

  /* It is not possible for a REDUCE to be followed by an error */
  assert( yyact!=YY_ERROR_ACTION );

  yymsp += yysize+1;
  yypParser->yytos = yymsp;
  yymsp->stateno = (YYACTIONTYPE)yyact;
  yymsp->major = (YYCODETYPE)yygoto;
  yyTraceShift(yypParser, yyact, "... then shift");
  return yyact;
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  grn_expr_parserARG_FETCH
  grn_expr_parserCTX_FETCH
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
/************ Begin %parse_failure code ***************************************/
/************ End %parse_failure code *****************************************/
  grn_expr_parserARG_STORE /* Suppress warning about unused %extra_argument variable */
  grn_expr_parserCTX_STORE
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  grn_expr_parserTOKENTYPE yyminor         /* The minor type of the error token */
){
  grn_expr_parserARG_FETCH
  grn_expr_parserCTX_FETCH
#define TOKEN yyminor
/************ Begin %syntax_error code ****************************************/
#line 20 "../../groonga/lib/grn_ecmascript.lemon"

  {
    grn_ctx *ctx = efsi->ctx;
    grn_obj message;
    GRN_TEXT_INIT(&message, 0);
    GRN_TEXT_PUT(ctx, &message, efsi->str, efsi->cur - efsi->str);
    GRN_TEXT_PUTC(ctx, &message, '|');
    if (efsi->cur < efsi->str_end) {
      GRN_TEXT_PUTC(ctx, &message, efsi->cur[0]);
      GRN_TEXT_PUTC(ctx, &message, '|');
      GRN_TEXT_PUT(ctx, &message,
                   efsi->cur + 1, efsi->str_end - (efsi->cur + 1));
    } else {
      GRN_TEXT_PUTC(ctx, &message, '|');
    }
    if (ctx->rc == GRN_SUCCESS) {
      ERR(GRN_SYNTAX_ERROR, "Syntax error: <%.*s>",
          (int)GRN_TEXT_LEN(&message), GRN_TEXT_VALUE(&message));
    } else {
      char errbuf[GRN_CTX_MSGSIZE];
      grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
      ERR(ctx->rc, "Syntax error: <%.*s>: %s",
          (int)GRN_TEXT_LEN(&message), GRN_TEXT_VALUE(&message),
          errbuf);
    }
    GRN_OBJ_FIN(ctx, &message);
  }
#line 2703 "../../groonga/lib/grn_ecmascript.c"
/************ End %syntax_error code ******************************************/
  grn_expr_parserARG_STORE /* Suppress warning about unused %extra_argument variable */
  grn_expr_parserCTX_STORE
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  grn_expr_parserARG_FETCH
  grn_expr_parserCTX_FETCH
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
#ifndef YYNOERRORRECOVERY
  yypParser->yyerrcnt = -1;
#endif
  assert( yypParser->yytos==yypParser->yystack );
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
/*********** Begin %parse_accept code *****************************************/
/*********** End %parse_accept code *******************************************/
  grn_expr_parserARG_STORE /* Suppress warning about unused %extra_argument variable */
  grn_expr_parserCTX_STORE
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
  YYACTIONTYPE yyact;   /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  int yyendofinput;     /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser = (yyParser*)yyp;  /* The parser */
  grn_expr_parserCTX_FETCH
  grn_expr_parserARG_STORE

  assert( yypParser->yytos!=0 );
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  yyendofinput = (yymajor==0);
#endif

  yyact = yypParser->yytos->stateno;
#ifndef NDEBUG
  if( yyTraceFILE ){
    if( yyact < YY_MIN_REDUCE ){
      fprintf(yyTraceFILE,"%sInput '%s' in state %d\n",
              yyTracePrompt,yyTokenName[yymajor],yyact);
    }else{
      fprintf(yyTraceFILE,"%sInput '%s' with pending reduce %d\n",
              yyTracePrompt,yyTokenName[yymajor],yyact-YY_MIN_REDUCE);
    }
  }
#endif

  do{
    assert( yyact==yypParser->yytos->stateno );
    yyact = yy_find_shift_action((YYCODETYPE)yymajor,yyact);
    if( yyact >= YY_MIN_REDUCE ){
      yyact = yy_reduce(yypParser,yyact-YY_MIN_REDUCE,yymajor,
                        yyminor grn_expr_parserCTX_PARAM);
    }else if( yyact <= YY_MAX_SHIFTREDUCE ){
      yy_shift(yypParser,yyact,(YYCODETYPE)yymajor,yyminor);
#ifndef YYNOERRORRECOVERY
      yypParser->yyerrcnt--;
#endif
      break;
    }else if( yyact==YY_ACCEPT_ACTION ){
      yypParser->yytos--;
      yy_accept(yypParser);
      return;
    }else{
      assert( yyact == YY_ERROR_ACTION );
      yyminorunion.yy0 = yyminor;
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
        yy_syntax_error(yypParser,yymajor,yyminor);
      }
      yymx = yypParser->yytos->major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor, &yyminorunion);
        yymajor = YYNOCODE;
      }else{
        while( yypParser->yytos >= yypParser->yystack
            && (yyact = yy_find_reduce_action(
                        yypParser->yytos->stateno,
                        YYERRORSYMBOL)) > YY_MAX_SHIFTREDUCE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yytos < yypParser->yystack || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
#ifndef YYNOERRORRECOVERY
          yypParser->yyerrcnt = -1;
#endif
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          yy_shift(yypParser,yyact,YYERRORSYMBOL,yyminor);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
      if( yymajor==YYNOCODE ) break;
      yyact = yypParser->yytos->stateno;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor, yyminor);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      break;
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
        yy_syntax_error(yypParser,yymajor, yyminor);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
#ifndef YYNOERRORRECOVERY
        yypParser->yyerrcnt = -1;
#endif
      }
      break;
#endif
    }
  }while( yypParser->yytos>yypParser->yystack );
#ifndef NDEBUG
  if( yyTraceFILE ){
    yyStackEntry *i;
    char cDiv = '[';
    fprintf(yyTraceFILE,"%sReturn. Stack=",yyTracePrompt);
    for(i=&yypParser->yystack[1]; i<=yypParser->yytos; i++){
      fprintf(yyTraceFILE,"%c%s", cDiv, yyTokenName[i->major]);
      cDiv = ' ';
    }
    fprintf(yyTraceFILE,"]\n");
  }
#endif
  return;
}

/*
** Return the fallback token corresponding to canonical token iToken, or
** 0 if iToken has no fallback.
*/
int grn_expr_parserFallback(int iToken){
#ifdef YYFALLBACK
  if( iToken<(int)(sizeof(yyFallback)/sizeof(yyFallback[0])) ){
    return yyFallback[iToken];
  }
#else
  (void)iToken;
#endif
  return 0;
}
