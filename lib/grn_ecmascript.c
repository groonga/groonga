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
/************ Begin %include sections from the grammar ************************/
#line 4 "grn_ecmascript.lemon"

#ifdef assert
#  undef assert
#endif
#define assert GRN_ASSERT
#line 34 "grn_ecmascript.c"
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
**    grn_expr_parserARG_STORE     Code to store %extra_argument into yypParser
**    grn_expr_parserARG_FETCH     Code to extract %extra_argument from yypParser
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YY_MAX_SHIFT       Maximum value for shift actions
**    YY_MIN_SHIFTREDUCE Minimum value for shift-reduce actions
**    YY_MAX_SHIFTREDUCE Maximum value for shift-reduce actions
**    YY_MIN_REDUCE      Maximum value for reduce actions
**    YY_ERROR_ACTION    The yy_action[] code for syntax error
**    YY_ACCEPT_ACTION   The yy_action[] code for accept
**    YY_NO_ACTION       The yy_action[] code for no-op
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/************* Begin control #defines *****************************************/
#define YYCODETYPE unsigned char
#define YYNOCODE 115
#define YYACTIONTYPE unsigned short int
#define grn_expr_parserTOKENTYPE  int 
typedef union {
  int yyinit;
  grn_expr_parserTOKENTYPE yy0;
  void * yy217;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define grn_expr_parserARG_SDECL  efs_info *efsi ;
#define grn_expr_parserARG_PDECL , efs_info *efsi 
#define grn_expr_parserARG_FETCH  efs_info *efsi  = yypParser->efsi 
#define grn_expr_parserARG_STORE yypParser->efsi  = efsi 
#define YYNSTATE             146
#define YYNRULE              133
#define YY_MAX_SHIFT         145
#define YY_MIN_SHIFTREDUCE   228
#define YY_MAX_SHIFTREDUCE   360
#define YY_MIN_REDUCE        361
#define YY_MAX_REDUCE        493
#define YY_ERROR_ACTION      494
#define YY_ACCEPT_ACTION     495
#define YY_NO_ACTION         496
/************* End control #defines *******************************************/

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
**   N between YY_MIN_REDUCE            Reduce by rule N-YY_MIN_REDUCE
**     and YY_MAX_REDUCE
**
**   N == YY_ERROR_ACTION               A syntax error has occurred.
**
**   N == YY_ACCEPT_ACTION              The parser accepts its input.
**
**   N == YY_NO_ACTION                  No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as either:
**
**    (A)   N = yy_action[ yy_shift_ofst[S] + X ]
**    (B)   N = yy_default[S]
**
** The (A) formula is preferred.  The B formula is used instead if:
**    (1)  The yy_shift_ofst[S]+X value is out of range, or
**    (2)  yy_lookahead[yy_shift_ofst[S]+X] is not equal to X, or
**    (3)  yy_shift_ofst[S] equal YY_SHIFT_USE_DFLT.
** (Implementation note: YY_SHIFT_USE_DFLT is chosen so that
** YY_SHIFT_USE_DFLT+X will be out of range for all possible lookaheads X.
** Hence only tests (1) and (2) need to be evaluated.)
**
** The formulas above are for computing the action when the lookahead is
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
**
*********** Begin parsing tables **********************************************/
#define YY_ACTTAB_COUNT (1683)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */     3,   72,  115,  115,  137,  132,  323,    2,  356,   54,
 /*    10 */    83,  130,    1,  228,   71,  495,   79,  112,  489,  237,
 /*    20 */    79,   75,  112,  112,   90,  127,  126,  140,  139,  138,
 /*    30 */   121,   87,  103,  117,  104,  104,  104,   90,   75,  237,
 /*    40 */   237,   75,   75,  323,   74,  232,   84,   83,  145,    9,
 /*    50 */   231,   71,  235,   66,   65,   53,   52,   51,   69,   68,
 /*    60 */    67,   64,   63,   61,   60,   59,  348,  349,  350,  351,
 /*    70 */   352,    6,  128,   70,   58,   57,   75,  128,  128,   90,
 /*    80 */   127,  126,  140,  139,  138,  121,   87,  103,  117,  104,
 /*    90 */   104,  104,   90,   75,   78,  454,   75,   75,   78,  115,
 /*   100 */   115,  137,  296,  343,  323,    2,  110,   54,   83,  130,
 /*   110 */     1,  234,   71,   78,  119,  315,  134,   78,   75,  119,
 /*   120 */   119,   90,  127,  126,  140,  139,  138,  121,   87,  103,
 /*   130 */   117,  104,  104,  104,   90,   75,  110,  134,   75,   75,
 /*   140 */     7,  300,   62,   77,  346,   73,  355,  137,  304,   76,
 /*   150 */   233,   66,   65,   30,   29,   21,   69,   68,   67,   64,
 /*   160 */    63,   61,   60,   59,  348,  349,  350,  351,  352,    6,
 /*   170 */    50,   49,   48,   47,   46,   45,   44,   43,   42,   41,
 /*   180 */    40,   39,   38,   37,   36,  230,   66,   65,  312,   56,
 /*   190 */    55,   69,   68,   67,   64,   63,   61,   60,   59,  348,
 /*   200 */   349,  350,  351,  352,    6,  111,  229,  313,   75,  314,
 /*   210 */   314,   90,  127,  126,  140,  139,  138,  121,   87,  103,
 /*   220 */   117,  104,  104,  104,   90,   75,  345,  317,   75,   75,
 /*   230 */     5,   28,   23,   27,   82,  309,   75,  314,  314,   90,
 /*   240 */   127,  126,  140,  139,  138,  121,   87,  103,  117,  104,
 /*   250 */   104,  104,   90,   75,  453,   25,   75,   75,   35,   34,
 /*   260 */   294,  295,  236,  311,   27,  316,  131,  133,  357,   31,
 /*   270 */     4,   75,  306,  306,   90,  127,  126,  140,  139,  138,
 /*   280 */   121,   87,  103,  117,  104,  104,  104,   90,   75,  297,
 /*   290 */    27,   75,   75,   22,    8,   33,  136,  114,   75,  299,
 /*   300 */   299,   90,  127,  126,  140,  139,  138,  121,   87,  103,
 /*   310 */   117,  104,  104,  104,   90,   75,   32,  119,   75,   75,
 /*   320 */   116,   75,  119,  119,   90,  127,  126,  140,  139,  138,
 /*   330 */   121,   87,  103,  117,  104,  104,  104,   90,   75,   24,
 /*   340 */   363,   75,   75,  363,  122,  363,  363,  303,   75,  122,
 /*   350 */   122,   90,  127,  126,  140,  139,  138,  121,   87,  103,
 /*   360 */   117,  104,  104,  104,   90,   75,  128,  363,   75,   75,
 /*   370 */    75,  128,  128,   90,  127,  126,  140,  139,  138,  121,
 /*   380 */    87,  103,  117,  104,  104,  104,   90,   75,  363,  363,
 /*   390 */    75,   75,    7,  363,   62,  363,  346,   73,   75,  250,
 /*   400 */   250,   90,  127,  126,  140,  139,  138,  121,   87,  103,
 /*   410 */   117,  104,  104,  104,   90,   75,  363,  363,   75,   75,
 /*   420 */    75,  249,  249,   90,  127,  126,  140,  139,  138,  121,
 /*   430 */    87,  103,  117,  104,  104,  104,   90,   75,   66,   65,
 /*   440 */    75,   75,  452,   69,   68,   67,   64,   63,   61,   60,
 /*   450 */    59,  348,  349,  129,  351,  352,    6,    7,   27,   62,
 /*   460 */   363,  346,   73,   75,  248,  248,   90,  127,  126,  140,
 /*   470 */   139,  138,  121,   87,  103,  117,  104,  104,  104,   90,
 /*   480 */    75,  363,  363,   75,   75,   75,  247,  247,   90,  127,
 /*   490 */   126,  140,  139,  138,  121,   87,  103,  117,  104,  104,
 /*   500 */   104,   90,   75,   66,   65,   75,   75,  363,   69,   68,
 /*   510 */    67,   64,   63,   61,   60,   59,  348,  349,  350,  351,
 /*   520 */   352,    6,   75,  246,  246,   90,  127,  126,  140,  139,
 /*   530 */   138,  121,   87,  103,  117,  104,  104,  104,   90,   75,
 /*   540 */   363,  363,   75,   75,   75,  245,  245,   90,  127,  126,
 /*   550 */   140,  139,  138,  121,   87,  103,  117,  104,  104,  104,
 /*   560 */    90,   75,  363,  363,   75,   75,  363,  363,   75,  244,
 /*   570 */   244,   90,  127,  126,  140,  139,  138,  121,   87,  103,
 /*   580 */   117,  104,  104,  104,   90,   75,  363,  363,   75,   75,
 /*   590 */    75,  243,  243,   90,  127,  126,  140,  139,  138,  121,
 /*   600 */    87,  103,  117,  104,  104,  104,   90,   75,  363,  363,
 /*   610 */    75,   75,   75,  242,  242,   90,  127,  126,  140,  139,
 /*   620 */   138,  121,   87,  103,  117,  104,  104,  104,   90,   75,
 /*   630 */   363,  363,   75,   75,   75,  241,  241,   90,  127,  126,
 /*   640 */   140,  139,  138,  121,   87,  103,  117,  104,  104,  104,
 /*   650 */    90,   75,  363,  363,   75,   75,   75,  240,  240,   90,
 /*   660 */   127,  126,  140,  139,  138,  121,   87,  103,  117,  104,
 /*   670 */   104,  104,   90,   75,  363,  363,   75,   75,   75,  307,
 /*   680 */   307,   90,  127,  126,  140,  139,  138,  121,   87,  103,
 /*   690 */   117,  104,  104,  104,   90,   75,  363,  363,   75,   75,
 /*   700 */    75,  302,  302,   90,  127,  126,  140,  139,  138,  121,
 /*   710 */    87,  103,  117,  104,  104,  104,   90,   75,  363,  363,
 /*   720 */    75,   75,   75,  353,  353,   90,  127,  126,  140,  139,
 /*   730 */   138,  121,   87,  103,  117,  104,  104,  104,   90,   75,
 /*   740 */   363,  363,   75,   75,   75,  251,  251,   90,  127,  126,
 /*   750 */   140,  139,  138,  121,   87,  103,  117,  104,  104,  104,
 /*   760 */    90,   75,  363,  363,   75,   75,   75,  144,  144,   90,
 /*   770 */   127,  126,  140,  139,  138,  121,   87,  103,  117,  104,
 /*   780 */   104,  104,   90,   75,  363,  363,   75,   75,   75,  239,
 /*   790 */   239,   90,  127,  126,  140,  139,  138,  121,   87,  103,
 /*   800 */   117,  104,  104,  104,   90,   75,  363,  363,   75,   75,
 /*   810 */    75,  238,  238,   90,  127,  126,  140,  139,  138,  121,
 /*   820 */    87,  103,  117,  104,  104,  104,   90,   75,  363,   75,
 /*   830 */    75,   75,  123,  363,  113,  140,  139,  138,  121,   87,
 /*   840 */   103,  117,  104,  104,  104,  123,   75,  363,  363,   75,
 /*   850 */    75,   75,  363,  363,  123,  363,  363,  135,  139,  138,
 /*   860 */   121,   87,  103,  117,  104,  104,  104,  123,   75,  363,
 /*   870 */   363,   75,   75,   75,  363,  363,  123,  363,  363,  143,
 /*   880 */   139,  138,  121,   87,  103,  117,  104,  104,  104,  123,
 /*   890 */    75,  363,  363,   75,   75,   75,  363,  363,  123,  363,
 /*   900 */   363,  363,  142,  138,  121,   87,  103,  117,  104,  104,
 /*   910 */   104,  123,   75,  363,   75,   75,   75,  123,  363,  363,
 /*   920 */   363,  363,  141,  121,   87,  103,  117,  104,  104,  104,
 /*   930 */   123,   75,  363,  363,   75,   75,  363,  363,  363,  363,
 /*   940 */    26,   20,   19,   18,   17,   16,   15,   14,   13,   12,
 /*   950 */    11,   10,   75,  363,  363,  123,  363,  363,  363,  363,
 /*   960 */   363,  125,   87,  103,  117,  104,  104,  104,  123,   75,
 /*   970 */   363,  363,   75,   75,  363,   75,  363,  363,  123,  363,
 /*   980 */   363,  363,  363,  294,  295,   88,  103,  117,  104,  104,
 /*   990 */   104,  123,   75,  363,   75,   75,   75,  123,  363,  363,
 /*  1000 */   363,  363,  363,  363,   89,  103,  117,  104,  104,  104,
 /*  1010 */   123,   75,  363,  363,   75,   75,  363,  363,   86,   85,
 /*  1020 */    81,   80,  323,   74,  324,   84,   83,  145,    9,  451,
 /*  1030 */    71,  363,   86,   85,   81,   80,  323,   74,  363,   84,
 /*  1040 */    83,  145,    9,  363,   71,  363,   75,  363,  363,  123,
 /*  1050 */   363,  363,  363,  363,  363,  363,  363,   91,  117,  104,
 /*  1060 */   104,  104,  123,   75,  363,  363,   75,   75,   75,  363,
 /*  1070 */   363,  123,  363,  363,  363,  363,  363,  363,  363,   92,
 /*  1080 */   117,  104,  104,  104,  123,   75,  363,  363,   75,   75,
 /*  1090 */    75,  363,  363,  123,  363,  363,  363,  363,  363,  363,
 /*  1100 */   363,   93,  117,  104,  104,  104,  123,   75,  363,   75,
 /*  1110 */    75,   75,  123,  363,  363,  363,  363,  363,  363,  363,
 /*  1120 */    94,  117,  104,  104,  104,  123,   75,  363,  363,   75,
 /*  1130 */    75,   75,  363,  363,  123,  363,  363,  363,  363,  363,
 /*  1140 */   363,  363,   95,  117,  104,  104,  104,  123,   75,  363,
 /*  1150 */   363,   75,   75,   75,  363,  363,  123,  363,  363,  363,
 /*  1160 */   363,  363,  363,  363,   96,  117,  104,  104,  104,  123,
 /*  1170 */    75,  363,  363,   75,   75,   75,  363,  363,  123,  363,
 /*  1180 */   363,  363,  363,  363,  363,  363,   97,  117,  104,  104,
 /*  1190 */   104,  123,   75,  363,   75,   75,   75,  123,  363,  363,
 /*  1200 */   363,  363,  363,  363,  363,   98,  117,  104,  104,  104,
 /*  1210 */   123,   75,  363,  363,   75,   75,   75,  363,  363,  123,
 /*  1220 */   363,  363,  363,  363,  363,  363,  363,   99,  117,  104,
 /*  1230 */   104,  104,  123,   75,  363,  363,   75,   75,   75,  363,
 /*  1240 */   363,  123,  363,  363,  363,  363,  363,  363,  363,  100,
 /*  1250 */   117,  104,  104,  104,  123,   75,  363,  363,   75,   75,
 /*  1260 */    75,  363,  363,  123,  363,  363,  363,  363,  363,  363,
 /*  1270 */   363,  101,  117,  104,  104,  104,  123,   75,  363,   75,
 /*  1280 */    75,   75,  123,  363,  363,  363,  363,  363,  363,  363,
 /*  1290 */   102,  117,  104,  104,  104,  123,   75,  363,  363,   75,
 /*  1300 */    75,   75,  363,  363,  123,  363,  363,  363,  363,  363,
 /*  1310 */   363,  363,  105,  117,  104,  104,  104,  123,   75,  363,
 /*  1320 */   363,   75,   75,   75,  363,  363,  123,  363,  363,  363,
 /*  1330 */   363,  363,  363,  363,  107,  117,  104,  104,  104,  123,
 /*  1340 */    75,  363,  363,   75,   75,   75,  363,  363,  123,  363,
 /*  1350 */   363,  363,  363,  363,  363,  363,  109,  117,  104,  104,
 /*  1360 */   104,  123,   75,  363,   75,   75,   75,  123,  363,  363,
 /*  1370 */   363,  363,  363,  363,  363,  363,  118,  104,  104,  104,
 /*  1380 */   123,   75,  363,  363,   75,   75,   75,  363,  363,  123,
 /*  1390 */   363,  363,  363,  363,  363,  363,  363,  363,  120,  104,
 /*  1400 */   104,  104,  123,   75,  363,  363,   75,   75,   75,    7,
 /*  1410 */   363,  123,  363,  346,   73,  363,  363,  363,  363,  363,
 /*  1420 */   124,  104,  104,  104,  123,   75,  363,  363,   75,   75,
 /*  1430 */   363,  363,  233,   75,  363,  363,  123,  363,  363,  363,
 /*  1440 */   363,  363,   75,  363,  363,  123,  363,  290,  290,  123,
 /*  1450 */    75,  363,  363,   75,   75,  106,  106,  106,  123,   75,
 /*  1460 */   363,   75,   75,   75,  123,  363,  363,  363,  348,  349,
 /*  1470 */   350,  351,  352,    6,  108,  108,  108,  123,   75,  363,
 /*  1480 */   363,   75,   75,   75,  363,  363,  123,  363,  363,  363,
 /*  1490 */   363,   75,  363,  363,  123,  363,  363,  282,  282,  123,
 /*  1500 */    75,  363,  363,   75,   75,  281,  281,  123,   75,  363,
 /*  1510 */    75,   75,   75,  123,  363,  363,  363,  363,   75,  363,
 /*  1520 */   363,  123,  363,  363,  293,  293,  123,   75,  363,  363,
 /*  1530 */    75,   75,  292,  292,  123,   75,  363,  363,   75,   75,
 /*  1540 */    75,  363,  363,  123,  363,  363,  363,  363,   75,  363,
 /*  1550 */   363,  123,  363,  363,  291,  291,  123,   75,  363,  363,
 /*  1560 */    75,   75,  290,  290,  123,   75,  363,   75,   75,   75,
 /*  1570 */   123,  363,  363,  363,  363,  363,   75,  363,  363,  123,
 /*  1580 */   363,  289,  289,  123,   75,  363,  363,   75,   75,  363,
 /*  1590 */   288,  288,  123,   75,  363,   75,   75,   75,  123,  363,
 /*  1600 */   363,  363,  363,   75,  363,  363,  123,  363,  363,  287,
 /*  1610 */   287,  123,   75,  363,  363,   75,   75,  286,  286,  123,
 /*  1620 */    75,  363,  363,   75,   75,   75,  363,  363,  123,  363,
 /*  1630 */   363,  363,  363,   75,  363,  363,  123,  363,  363,  285,
 /*  1640 */   285,  123,   75,  363,  363,   75,   75,  284,  284,  123,
 /*  1650 */    75,  363,   75,   75,   75,  123,  363,  363,  363,  363,
 /*  1660 */   363,   75,  363,  363,  123,  363,  283,  283,  123,   75,
 /*  1670 */   363,  363,   75,   75,  363,  280,  280,  123,   75,  363,
 /*  1680 */   363,   75,   75,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     1,    2,  107,  108,  109,   12,    7,    8,   69,   10,
 /*    10 */    11,   12,   13,   83,   15,   78,   79,   80,   30,   84,
 /*    20 */    83,   84,   85,   86,   87,   88,   89,   90,   91,   92,
 /*    30 */    93,   94,   95,   96,   97,   98,   99,  100,  101,  104,
 /*    40 */   105,  104,  105,    7,    8,   83,   10,   11,   12,   13,
 /*    50 */    83,   15,   83,   54,   55,   51,   52,   53,   59,   60,
 /*    60 */    61,   62,   63,   64,   65,   66,   67,   68,   69,   70,
 /*    70 */    71,   72,   80,   56,   57,   58,   84,   85,   86,   87,
 /*    80 */    88,   89,   90,   91,   92,   93,   94,   95,   96,   97,
 /*    90 */    98,   99,  100,  101,   79,    0,  104,  105,   83,  107,
 /*   100 */   108,  109,  102,  103,    7,    8,   82,   10,   11,   12,
 /*   110 */    13,   83,   15,   79,   80,  112,  113,   83,   84,   85,
 /*   120 */    86,   87,   88,   89,   90,   91,   92,   93,   94,   95,
 /*   130 */    96,   97,   98,   99,  100,  101,  112,  113,  104,  105,
 /*   140 */     8,   14,   10,   16,   12,   13,  108,  109,    9,   54,
 /*   150 */    83,   54,   55,    3,    4,   16,   59,   60,   61,   62,
 /*   160 */    63,   64,   65,   66,   67,   68,   69,   70,   71,   72,
 /*   170 */    36,   37,   38,   39,   40,   41,   42,   43,   44,   45,
 /*   180 */    46,   47,   48,   49,   50,   83,   54,   55,   56,   54,
 /*   190 */    55,   59,   60,   61,   62,   63,   64,   65,   66,   67,
 /*   200 */    68,   69,   70,   71,   72,   81,   83,   75,   84,   85,
 /*   210 */    86,   87,   88,   89,   90,   91,   92,   93,   94,   95,
 /*   220 */    96,   97,   98,   99,  100,  101,    9,   69,  104,  105,
 /*   230 */     8,    5,   16,   16,   11,  111,   84,   85,   86,   87,
 /*   240 */    88,   89,   90,   91,   92,   93,   94,   95,   96,   97,
 /*   250 */    98,   99,  100,  101,    0,   29,  104,  105,   34,   35,
 /*   260 */    60,   61,   14,  111,   16,   67,   41,   56,   12,   31,
 /*   270 */    16,   84,   85,   86,   87,   88,   89,   90,   91,   92,
 /*   280 */    93,   94,   95,   96,   97,   98,   99,  100,  101,   73,
 /*   290 */    16,  104,  105,   30,   72,   33,   74,  110,   84,   85,
 /*   300 */    86,   87,   88,   89,   90,   91,   92,   93,   94,   95,
 /*   310 */    96,   97,   98,   99,  100,  101,   32,   80,  104,  105,
 /*   320 */   106,   84,   85,   86,   87,   88,   89,   90,   91,   92,
 /*   330 */    93,   94,   95,   96,   97,   98,   99,  100,  101,   30,
 /*   340 */   114,  104,  105,  114,   80,  114,  114,   73,   84,   85,
 /*   350 */    86,   87,   88,   89,   90,   91,   92,   93,   94,   95,
 /*   360 */    96,   97,   98,   99,  100,  101,   80,  114,  104,  105,
 /*   370 */    84,   85,   86,   87,   88,   89,   90,   91,   92,   93,
 /*   380 */    94,   95,   96,   97,   98,   99,  100,  101,  114,  114,
 /*   390 */   104,  105,    8,  114,   10,  114,   12,   13,   84,   85,
 /*   400 */    86,   87,   88,   89,   90,   91,   92,   93,   94,   95,
 /*   410 */    96,   97,   98,   99,  100,  101,  114,  114,  104,  105,
 /*   420 */    84,   85,   86,   87,   88,   89,   90,   91,   92,   93,
 /*   430 */    94,   95,   96,   97,   98,   99,  100,  101,   54,   55,
 /*   440 */   104,  105,    0,   59,   60,   61,   62,   63,   64,   65,
 /*   450 */    66,   67,   68,   69,   70,   71,   72,    8,   16,   10,
 /*   460 */   114,   12,   13,   84,   85,   86,   87,   88,   89,   90,
 /*   470 */    91,   92,   93,   94,   95,   96,   97,   98,   99,  100,
 /*   480 */   101,  114,  114,  104,  105,   84,   85,   86,   87,   88,
 /*   490 */    89,   90,   91,   92,   93,   94,   95,   96,   97,   98,
 /*   500 */    99,  100,  101,   54,   55,  104,  105,  114,   59,   60,
 /*   510 */    61,   62,   63,   64,   65,   66,   67,   68,   69,   70,
 /*   520 */    71,   72,   84,   85,   86,   87,   88,   89,   90,   91,
 /*   530 */    92,   93,   94,   95,   96,   97,   98,   99,  100,  101,
 /*   540 */   114,  114,  104,  105,   84,   85,   86,   87,   88,   89,
 /*   550 */    90,   91,   92,   93,   94,   95,   96,   97,   98,   99,
 /*   560 */   100,  101,  114,  114,  104,  105,  114,  114,   84,   85,
 /*   570 */    86,   87,   88,   89,   90,   91,   92,   93,   94,   95,
 /*   580 */    96,   97,   98,   99,  100,  101,  114,  114,  104,  105,
 /*   590 */    84,   85,   86,   87,   88,   89,   90,   91,   92,   93,
 /*   600 */    94,   95,   96,   97,   98,   99,  100,  101,  114,  114,
 /*   610 */   104,  105,   84,   85,   86,   87,   88,   89,   90,   91,
 /*   620 */    92,   93,   94,   95,   96,   97,   98,   99,  100,  101,
 /*   630 */   114,  114,  104,  105,   84,   85,   86,   87,   88,   89,
 /*   640 */    90,   91,   92,   93,   94,   95,   96,   97,   98,   99,
 /*   650 */   100,  101,  114,  114,  104,  105,   84,   85,   86,   87,
 /*   660 */    88,   89,   90,   91,   92,   93,   94,   95,   96,   97,
 /*   670 */    98,   99,  100,  101,  114,  114,  104,  105,   84,   85,
 /*   680 */    86,   87,   88,   89,   90,   91,   92,   93,   94,   95,
 /*   690 */    96,   97,   98,   99,  100,  101,  114,  114,  104,  105,
 /*   700 */    84,   85,   86,   87,   88,   89,   90,   91,   92,   93,
 /*   710 */    94,   95,   96,   97,   98,   99,  100,  101,  114,  114,
 /*   720 */   104,  105,   84,   85,   86,   87,   88,   89,   90,   91,
 /*   730 */    92,   93,   94,   95,   96,   97,   98,   99,  100,  101,
 /*   740 */   114,  114,  104,  105,   84,   85,   86,   87,   88,   89,
 /*   750 */    90,   91,   92,   93,   94,   95,   96,   97,   98,   99,
 /*   760 */   100,  101,  114,  114,  104,  105,   84,   85,   86,   87,
 /*   770 */    88,   89,   90,   91,   92,   93,   94,   95,   96,   97,
 /*   780 */    98,   99,  100,  101,  114,  114,  104,  105,   84,   85,
 /*   790 */    86,   87,   88,   89,   90,   91,   92,   93,   94,   95,
 /*   800 */    96,   97,   98,   99,  100,  101,  114,  114,  104,  105,
 /*   810 */    84,   85,   86,   87,   88,   89,   90,   91,   92,   93,
 /*   820 */    94,   95,   96,   97,   98,   99,  100,  101,  114,   84,
 /*   830 */   104,  105,   87,  114,   89,   90,   91,   92,   93,   94,
 /*   840 */    95,   96,   97,   98,   99,  100,  101,  114,  114,  104,
 /*   850 */   105,   84,  114,  114,   87,  114,  114,   90,   91,   92,
 /*   860 */    93,   94,   95,   96,   97,   98,   99,  100,  101,  114,
 /*   870 */   114,  104,  105,   84,  114,  114,   87,  114,  114,   90,
 /*   880 */    91,   92,   93,   94,   95,   96,   97,   98,   99,  100,
 /*   890 */   101,  114,  114,  104,  105,   84,  114,  114,   87,  114,
 /*   900 */   114,  114,   91,   92,   93,   94,   95,   96,   97,   98,
 /*   910 */    99,  100,  101,  114,   84,  104,  105,   87,  114,  114,
 /*   920 */   114,  114,   92,   93,   94,   95,   96,   97,   98,   99,
 /*   930 */   100,  101,  114,  114,  104,  105,  114,  114,  114,  114,
 /*   940 */    17,   18,   19,   20,   21,   22,   23,   24,   25,   26,
 /*   950 */    27,   28,   84,  114,  114,   87,  114,  114,  114,  114,
 /*   960 */   114,   93,   94,   95,   96,   97,   98,   99,  100,  101,
 /*   970 */   114,  114,  104,  105,  114,   84,  114,  114,   87,  114,
 /*   980 */   114,  114,  114,   60,   61,   94,   95,   96,   97,   98,
 /*   990 */    99,  100,  101,  114,   84,  104,  105,   87,  114,  114,
 /*  1000 */   114,  114,  114,  114,   94,   95,   96,   97,   98,   99,
 /*  1010 */   100,  101,  114,  114,  104,  105,  114,  114,    3,    4,
 /*  1020 */     5,    6,    7,    8,    9,   10,   11,   12,   13,    0,
 /*  1030 */    15,  114,    3,    4,    5,    6,    7,    8,  114,   10,
 /*  1040 */    11,   12,   13,  114,   15,  114,   84,  114,  114,   87,
 /*  1050 */   114,  114,  114,  114,  114,  114,  114,   95,   96,   97,
 /*  1060 */    98,   99,  100,  101,  114,  114,  104,  105,   84,  114,
 /*  1070 */   114,   87,  114,  114,  114,  114,  114,  114,  114,   95,
 /*  1080 */    96,   97,   98,   99,  100,  101,  114,  114,  104,  105,
 /*  1090 */    84,  114,  114,   87,  114,  114,  114,  114,  114,  114,
 /*  1100 */   114,   95,   96,   97,   98,   99,  100,  101,  114,   84,
 /*  1110 */   104,  105,   87,  114,  114,  114,  114,  114,  114,  114,
 /*  1120 */    95,   96,   97,   98,   99,  100,  101,  114,  114,  104,
 /*  1130 */   105,   84,  114,  114,   87,  114,  114,  114,  114,  114,
 /*  1140 */   114,  114,   95,   96,   97,   98,   99,  100,  101,  114,
 /*  1150 */   114,  104,  105,   84,  114,  114,   87,  114,  114,  114,
 /*  1160 */   114,  114,  114,  114,   95,   96,   97,   98,   99,  100,
 /*  1170 */   101,  114,  114,  104,  105,   84,  114,  114,   87,  114,
 /*  1180 */   114,  114,  114,  114,  114,  114,   95,   96,   97,   98,
 /*  1190 */    99,  100,  101,  114,   84,  104,  105,   87,  114,  114,
 /*  1200 */   114,  114,  114,  114,  114,   95,   96,   97,   98,   99,
 /*  1210 */   100,  101,  114,  114,  104,  105,   84,  114,  114,   87,
 /*  1220 */   114,  114,  114,  114,  114,  114,  114,   95,   96,   97,
 /*  1230 */    98,   99,  100,  101,  114,  114,  104,  105,   84,  114,
 /*  1240 */   114,   87,  114,  114,  114,  114,  114,  114,  114,   95,
 /*  1250 */    96,   97,   98,   99,  100,  101,  114,  114,  104,  105,
 /*  1260 */    84,  114,  114,   87,  114,  114,  114,  114,  114,  114,
 /*  1270 */   114,   95,   96,   97,   98,   99,  100,  101,  114,   84,
 /*  1280 */   104,  105,   87,  114,  114,  114,  114,  114,  114,  114,
 /*  1290 */    95,   96,   97,   98,   99,  100,  101,  114,  114,  104,
 /*  1300 */   105,   84,  114,  114,   87,  114,  114,  114,  114,  114,
 /*  1310 */   114,  114,   95,   96,   97,   98,   99,  100,  101,  114,
 /*  1320 */   114,  104,  105,   84,  114,  114,   87,  114,  114,  114,
 /*  1330 */   114,  114,  114,  114,   95,   96,   97,   98,   99,  100,
 /*  1340 */   101,  114,  114,  104,  105,   84,  114,  114,   87,  114,
 /*  1350 */   114,  114,  114,  114,  114,  114,   95,   96,   97,   98,
 /*  1360 */    99,  100,  101,  114,   84,  104,  105,   87,  114,  114,
 /*  1370 */   114,  114,  114,  114,  114,  114,   96,   97,   98,   99,
 /*  1380 */   100,  101,  114,  114,  104,  105,   84,  114,  114,   87,
 /*  1390 */   114,  114,  114,  114,  114,  114,  114,  114,   96,   97,
 /*  1400 */    98,   99,  100,  101,  114,  114,  104,  105,   84,    8,
 /*  1410 */   114,   87,  114,   12,   13,  114,  114,  114,  114,  114,
 /*  1420 */    96,   97,   98,   99,  100,  101,  114,  114,  104,  105,
 /*  1430 */   114,  114,   83,   84,  114,  114,   87,  114,  114,  114,
 /*  1440 */   114,  114,   84,  114,  114,   87,  114,   98,   99,  100,
 /*  1450 */   101,  114,  114,  104,  105,   97,   98,   99,  100,  101,
 /*  1460 */   114,   84,  104,  105,   87,  114,  114,  114,   67,   68,
 /*  1470 */    69,   70,   71,   72,   97,   98,   99,  100,  101,  114,
 /*  1480 */   114,  104,  105,   84,  114,  114,   87,  114,  114,  114,
 /*  1490 */   114,   84,  114,  114,   87,  114,  114,   98,   99,  100,
 /*  1500 */   101,  114,  114,  104,  105,   98,   99,  100,  101,  114,
 /*  1510 */    84,  104,  105,   87,  114,  114,  114,  114,   84,  114,
 /*  1520 */   114,   87,  114,  114,   98,   99,  100,  101,  114,  114,
 /*  1530 */   104,  105,   98,   99,  100,  101,  114,  114,  104,  105,
 /*  1540 */    84,  114,  114,   87,  114,  114,  114,  114,   84,  114,
 /*  1550 */   114,   87,  114,  114,   98,   99,  100,  101,  114,  114,
 /*  1560 */   104,  105,   98,   99,  100,  101,  114,   84,  104,  105,
 /*  1570 */    87,  114,  114,  114,  114,  114,   84,  114,  114,   87,
 /*  1580 */   114,   98,   99,  100,  101,  114,  114,  104,  105,  114,
 /*  1590 */    98,   99,  100,  101,  114,   84,  104,  105,   87,  114,
 /*  1600 */   114,  114,  114,   84,  114,  114,   87,  114,  114,   98,
 /*  1610 */    99,  100,  101,  114,  114,  104,  105,   98,   99,  100,
 /*  1620 */   101,  114,  114,  104,  105,   84,  114,  114,   87,  114,
 /*  1630 */   114,  114,  114,   84,  114,  114,   87,  114,  114,   98,
 /*  1640 */    99,  100,  101,  114,  114,  104,  105,   98,   99,  100,
 /*  1650 */   101,  114,   84,  104,  105,   87,  114,  114,  114,  114,
 /*  1660 */   114,   84,  114,  114,   87,  114,   98,   99,  100,  101,
 /*  1670 */   114,  114,  104,  105,  114,   98,   99,  100,  101,  114,
 /*  1680 */   114,  104,  105,
};
#define YY_SHIFT_USE_DFLT (1683)
#define YY_SHIFT_COUNT    (145)
#define YY_SHIFT_MIN      (-61)
#define YY_SHIFT_MAX      (1401)
static const short yy_shift_ofst[] = {
 /*     0 */    -1,  384,   97,  132,  132,  449,  449,  449,  449,  449,
 /*    10 */   449,  449,  449,  449,  449,  449,  449,  449,  449,  449,
 /*    20 */   449,  449,  449,  449,  449,  449,  449,  449,  449,  449,
 /*    30 */   449,  449,  449,  449,  449,  449,  449,  449,  449,  449,
 /*    40 */   449,  449,  449,  449,  449,  449,  449,  449,  449,  449,
 /*    50 */   449,  449,  449,  449,   97,  449,  449,  449,  449,  449,
 /*    60 */   449,  449,  449,  449,  449,  449,  449,  449,  449,  449,
 /*    70 */   449, 1401,   -7,  -61,   36,  222,   -7,  -61, 1015, 1029,
 /*    80 */    36,   36,   36,   36,   36,   36,   36,  134,  134,  134,
 /*    90 */   923,    4,    4,    4,    4,    4,    4,    4,    4,    4,
 /*   100 */     4,    4,    4,    4,   17,    4,   17,    4,   17,    4,
 /*   110 */    95,  254,  442,  150,  139,  127,  216,  135,  135,  217,
 /*   120 */   135,  224,  274,  200,  135,  224,  150,  226,  248,  -12,
 /*   130 */   223,  158,  225,  198,  211,  238,  256,  263,  262,  284,
 /*   140 */   238,  262,  284,  238,  309,  223,
};
#define YY_REDUCE_USE_DFLT (-106)
#define YY_REDUCE_COUNT (86)
#define YY_REDUCE_MIN   (-105)
#define YY_REDUCE_MAX   (1577)
static const short yy_reduce_ofst[] = {
 /*     0 */   -63,   -8,   34,  124,  152,  187,  214,  237,  264,  286,
 /*    10 */   314,  336,  379,  401,  438,  460,  484,  506,  528,  550,
 /*    20 */   572,  594,  616,  638,  660,  682,  704,  726,  745,  767,
 /*    30 */   789,  811,  830,  868,  891,  910,  962,  984, 1006, 1025,
 /*    40 */  1047, 1069, 1091, 1110, 1132, 1154, 1176, 1195, 1217, 1239,
 /*    50 */  1261, 1280, 1302, 1324, 1349, 1358, 1377, 1399, 1407, 1426,
 /*    60 */  1434, 1456, 1464, 1483, 1492, 1511, 1519, 1541, 1549, 1568,
 /*    70 */  1577,  -65,   24, -105,   15,    0,    3,   38,  -70,  -70,
 /*    80 */   -38,  -33,  -31,   28,   67,  102,  123,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   494,  434,  494,  441,  443,  438,  431,  494,  494,  494,
 /*    10 */   494,  494,  494,  494,  494,  494,  494,  494,  494,  494,
 /*    20 */   494,  494,  494,  494,  494,  494,  494,  494,  494,  494,
 /*    30 */   494,  494,  494,  494,  494,  494,  494,  494,  494,  494,
 /*    40 */   494,  494,  494,  494,  494,  494,  494,  494,  494,  494,
 /*    50 */   494,  494,  494,  494,  494,  494,  494,  494,  494,  494,
 /*    60 */   494,  494,  494,  494,  494,  494,  494,  494,  494,  494,
 /*    70 */   494,  494,  491,  434,  494,  474,  494,  494,  494,  494,
 /*    80 */   494,  494,  494,  494,  494,  494,  494,  466,  392,  391,
 /*    90 */   472,  407,  406,  405,  404,  403,  402,  401,  400,  399,
 /*   100 */   398,  397,  396,  467,  469,  395,  412,  394,  411,  393,
 /*   110 */   494,  494,  494,  385,  494,  494,  494,  468,  410,  494,
 /*   120 */   409,  465,  494,  472,  408,  390,  461,  460,  494,  483,
 /*   130 */   479,  494,  494,  494,  493,  387,  494,  494,  464,  463,
 /*   140 */   462,  389,  388,  386,  494,  494,
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
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
  yyStackEntry yystk0;          /* First stack entry */
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
  "$",             "START_OUTPUT_COLUMNS",  "START_ADJUSTER",  "LOGICAL_AND", 
  "LOGICAL_AND_NOT",  "LOGICAL_OR",    "NEGATIVE",      "QSTRING",     
  "PARENL",        "PARENR",        "ADJUST",        "RELATIVE_OP", 
  "IDENTIFIER",    "BRACEL",        "BRACER",        "EVAL",        
  "COMMA",         "ASSIGN",        "STAR_ASSIGN",   "SLASH_ASSIGN",
  "MOD_ASSIGN",    "PLUS_ASSIGN",   "MINUS_ASSIGN",  "SHIFTL_ASSIGN",
  "SHIFTR_ASSIGN",  "SHIFTRR_ASSIGN",  "AND_ASSIGN",    "XOR_ASSIGN",  
  "OR_ASSIGN",     "QUESTION",      "COLON",         "BITWISE_OR",  
  "BITWISE_XOR",   "BITWISE_AND",   "EQUAL",         "NOT_EQUAL",   
  "LESS",          "GREATER",       "LESS_EQUAL",    "GREATER_EQUAL",
  "IN",            "MATCH",         "NEAR",          "NEAR2",       
  "SIMILAR",       "TERM_EXTRACT",  "QUORUM",        "LCP",         
  "PREFIX",        "SUFFIX",        "REGEXP",        "SHIFTL",      
  "SHIFTR",        "SHIFTRR",       "PLUS",          "MINUS",       
  "STAR",          "SLASH",         "MOD",           "DELETE",      
  "INCR",          "DECR",          "NOT",           "BITWISE_NOT", 
  "EXACT",         "PARTIAL",       "UNSPLIT",       "DECIMAL",     
  "HEX_INTEGER",   "STRING",        "BOOLEAN",       "NULL",        
  "BRACKETL",      "BRACKETR",      "DOT",           "NONEXISTENT_COLUMN",
  "error",         "suppress_unused_variable_warning",  "input",         "query",       
  "expression",    "output_columns",  "adjuster",      "query_element",
  "primary_expression",  "assignment_expression",  "conditional_expression",  "lefthand_side_expression",
  "logical_or_expression",  "logical_and_expression",  "bitwise_or_expression",  "bitwise_xor_expression",
  "bitwise_and_expression",  "equality_expression",  "relational_expression",  "shift_expression",
  "additive_expression",  "multiplicative_expression",  "unary_expression",  "postfix_expression",
  "call_expression",  "member_expression",  "arguments",     "member_expression_part",
  "object_literal",  "array_literal",  "element_list",  "property_name_and_value_list",
  "property_name_and_value",  "property_name",  "argument_list",  "output_column",
  "adjust_expression",  "adjust_match_expression",
};
#endif /* NDEBUG */

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
void *grn_expr_parserAlloc(void *(*mallocProc)(YYMALLOCARGTYPE)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (YYMALLOCARGTYPE)sizeof(yyParser) );
  if( pParser ){
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyhwm = 0;
#endif
#if YYSTACKDEPTH<=0
    pParser->yytos = NULL;
    pParser->yystack = NULL;
    pParser->yystksz = 0;
    if( yyGrowStack(pParser) ){
      pParser->yystack = &pParser->yystk0;
      pParser->yystksz = 1;
    }
#endif
#ifndef YYNOERRORRECOVERY
    pParser->yyerrcnt = -1;
#endif
    pParser->yytos = pParser->yystack;
    pParser->yystack[0].stateno = 0;
    pParser->yystack[0].major = 0;
  }
  return pParser;
}

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
  grn_expr_parserARG_FETCH;
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
    case 77: /* suppress_unused_variable_warning */
{
#line 14 "grn_ecmascript.lemon"

  (void)efsi;

#line 973 "grn_ecmascript.c"
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
  yyParser *pParser = (yyParser*)p;
#ifndef YYPARSEFREENEVERNULL
  if( pParser==0 ) return;
#endif
  while( pParser->yytos>pParser->yystack ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  if( pParser->yystack!=&pParser->yystk0 ) free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int grn_expr_parserStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyhwm;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
*/
static unsigned int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yytos->stateno;
 
  if( stateno>=YY_MIN_REDUCE ) return stateno;
  assert( stateno <= YY_SHIFT_COUNT );
  do{
    i = yy_shift_ofst[stateno];
    assert( iLookAhead!=YYNOCODE );
    i += iLookAhead;
    if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
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
static void yyStackOverflow(yyParser *yypParser){
   grn_expr_parserARG_FETCH;
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
   grn_expr_parserARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Print tracing information for a SHIFT action
*/
#ifndef NDEBUG
static void yyTraceShift(yyParser *yypParser, int yyNewState){
  if( yyTraceFILE ){
    if( yyNewState<YYNSTATE ){
      fprintf(yyTraceFILE,"%sShift '%s', go to state %d\n",
         yyTracePrompt,yyTokenName[yypParser->yytos->major],
         yyNewState);
    }else{
      fprintf(yyTraceFILE,"%sShift '%s'\n",
         yyTracePrompt,yyTokenName[yypParser->yytos->major]);
    }
  }
}
#else
# define yyTraceShift(X,Y)
#endif

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
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
  if( yypParser->yytos>=&yypParser->yystack[YYSTACKDEPTH] ){
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
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  yytos->minor.yy0 = yyMinor;
  yyTraceShift(yypParser, yyNewState);
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 79, 2 },
  { 79, 3 },
  { 79, 3 },
  { 79, 3 },
  { 79, 3 },
  { 83, 2 },
  { 83, 2 },
  { 83, 3 },
  { 83, 3 },
  { 83, 2 },
  { 80, 3 },
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
  { 86, 5 },
  { 88, 3 },
  { 89, 3 },
  { 89, 3 },
  { 90, 3 },
  { 91, 3 },
  { 92, 3 },
  { 93, 3 },
  { 93, 3 },
  { 94, 3 },
  { 94, 3 },
  { 94, 3 },
  { 94, 3 },
  { 94, 3 },
  { 94, 3 },
  { 94, 3 },
  { 94, 3 },
  { 94, 3 },
  { 94, 3 },
  { 94, 3 },
  { 94, 3 },
  { 94, 3 },
  { 94, 3 },
  { 94, 3 },
  { 95, 3 },
  { 95, 3 },
  { 95, 3 },
  { 96, 3 },
  { 96, 3 },
  { 97, 3 },
  { 97, 3 },
  { 97, 3 },
  { 98, 2 },
  { 98, 2 },
  { 98, 2 },
  { 98, 2 },
  { 98, 2 },
  { 98, 2 },
  { 98, 2 },
  { 98, 2 },
  { 98, 2 },
  { 98, 2 },
  { 98, 2 },
  { 99, 2 },
  { 99, 2 },
  { 100, 2 },
  { 105, 3 },
  { 106, 0 },
  { 106, 1 },
  { 104, 3 },
  { 107, 0 },
  { 108, 3 },
  { 103, 3 },
  { 102, 3 },
  { 110, 0 },
  { 110, 1 },
  { 110, 3 },
  { 81, 0 },
  { 81, 1 },
  { 81, 2 },
  { 81, 3 },
  { 111, 1 },
  { 111, 1 },
  { 111, 1 },
  { 82, 3 },
  { 112, 3 },
  { 113, 3 },
  { 78, 1 },
  { 78, 1 },
  { 78, 2 },
  { 78, 2 },
  { 79, 1 },
  { 83, 1 },
  { 83, 3 },
  { 80, 1 },
  { 85, 1 },
  { 86, 1 },
  { 88, 1 },
  { 89, 1 },
  { 90, 1 },
  { 91, 1 },
  { 92, 1 },
  { 93, 1 },
  { 94, 1 },
  { 95, 1 },
  { 96, 1 },
  { 97, 1 },
  { 98, 1 },
  { 99, 1 },
  { 87, 1 },
  { 87, 1 },
  { 101, 1 },
  { 101, 2 },
  { 84, 1 },
  { 84, 3 },
  { 84, 1 },
  { 84, 1 },
  { 84, 1 },
  { 84, 1 },
  { 84, 1 },
  { 84, 1 },
  { 84, 1 },
  { 106, 3 },
  { 107, 1 },
  { 107, 3 },
  { 109, 1 },
  { 103, 2 },
  { 82, 0 },
  { 82, 1 },
  { 112, 1 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  unsigned int yyruleno        /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  grn_expr_parserARG_FETCH;
  yymsp = yypParser->yytos;
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    yysize = yyRuleInfo[yyruleno].nrhs;
    fprintf(yyTraceFILE, "%sReduce [%s], go to state %d.\n", yyTracePrompt,
      yyRuleName[yyruleno], yymsp[-yysize].stateno);
  }
#endif /* NDEBUG */

  /* Check that the stack is large enough to grow by a single entry
  ** if the RHS of the rule is empty.  This ensures that there is room
  ** enough on the stack to push the LHS value */
  if( yyRuleInfo[yyruleno].nrhs==0 ){
#ifdef YYTRACKMAXSTACKDEPTH
    if( (int)(yypParser->yytos - yypParser->yystack)>yypParser->yyhwm ){
      yypParser->yyhwm++;
      assert( yypParser->yyhwm == (int)(yypParser->yytos - yypParser->yystack));
    }
#endif
#if YYSTACKDEPTH>0 
    if( yypParser->yytos>=&yypParser->yystack[YYSTACKDEPTH-1] ){
      yyStackOverflow(yypParser);
      return;
    }
#else
    if( yypParser->yytos>=&yypParser->yystack[yypParser->yystksz-1] ){
      if( yyGrowStack(yypParser) ){
        yyStackOverflow(yypParser);
        return;
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
#line 55 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, grn_int32_value_at(&efsi->op_stack, -1), 2);
}
#line 1417 "grn_ecmascript.c"
        break;
      case 1: /* query ::= query LOGICAL_AND query_element */
      case 25: /* logical_and_expression ::= logical_and_expression LOGICAL_AND bitwise_or_expression */ yytestcase(yyruleno==25);
#line 58 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND, 2);
}
#line 1425 "grn_ecmascript.c"
        break;
      case 2: /* query ::= query LOGICAL_AND_NOT query_element */
      case 26: /* logical_and_expression ::= logical_and_expression LOGICAL_AND_NOT bitwise_or_expression */ yytestcase(yyruleno==26);
#line 61 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND_NOT, 2);
}
#line 1433 "grn_ecmascript.c"
        break;
      case 3: /* query ::= query LOGICAL_OR query_element */
      case 24: /* logical_or_expression ::= logical_or_expression LOGICAL_OR logical_and_expression */ yytestcase(yyruleno==24);
#line 64 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_OR, 2);
}
#line 1441 "grn_ecmascript.c"
        break;
      case 4: /* query ::= query NEGATIVE query_element */
#line 67 "grn_ecmascript.lemon"
{
  int weight;
  GRN_INT32_POP(&efsi->weight_stack, weight);
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_ADJUST, 2);
}
#line 1450 "grn_ecmascript.c"
        break;
      case 5: /* query_element ::= ADJUST query_element */
#line 76 "grn_ecmascript.lemon"
{
  int weight;
  GRN_INT32_POP(&efsi->weight_stack, weight);
}
#line 1458 "grn_ecmascript.c"
        break;
      case 6: /* query_element ::= RELATIVE_OP query_element */
#line 80 "grn_ecmascript.lemon"
{
  int mode;
  GRN_INT32_POP(&efsi->mode_stack, mode);
}
#line 1466 "grn_ecmascript.c"
        break;
      case 7: /* query_element ::= IDENTIFIER RELATIVE_OP query_element */
#line 84 "grn_ecmascript.lemon"
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
#line 1499 "grn_ecmascript.c"
        break;
      case 8: /* query_element ::= BRACEL expression BRACER */
      case 9: /* query_element ::= EVAL primary_expression */ yytestcase(yyruleno==9);
#line 113 "grn_ecmascript.lemon"
{
  efsi->flags = efsi->default_flags;
}
#line 1507 "grn_ecmascript.c"
        break;
      case 10: /* expression ::= expression COMMA assignment_expression */
#line 121 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_COMMA, 2);
}
#line 1514 "grn_ecmascript.c"
        break;
      case 11: /* assignment_expression ::= lefthand_side_expression ASSIGN assignment_expression */
#line 126 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_ASSIGN, 2);
}
#line 1521 "grn_ecmascript.c"
        break;
      case 12: /* assignment_expression ::= lefthand_side_expression STAR_ASSIGN assignment_expression */
#line 129 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_STAR_ASSIGN, 2);
}
#line 1528 "grn_ecmascript.c"
        break;
      case 13: /* assignment_expression ::= lefthand_side_expression SLASH_ASSIGN assignment_expression */
#line 132 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SLASH_ASSIGN, 2);
}
#line 1535 "grn_ecmascript.c"
        break;
      case 14: /* assignment_expression ::= lefthand_side_expression MOD_ASSIGN assignment_expression */
#line 135 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MOD_ASSIGN, 2);
}
#line 1542 "grn_ecmascript.c"
        break;
      case 15: /* assignment_expression ::= lefthand_side_expression PLUS_ASSIGN assignment_expression */
#line 138 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PLUS_ASSIGN, 2);
}
#line 1549 "grn_ecmascript.c"
        break;
      case 16: /* assignment_expression ::= lefthand_side_expression MINUS_ASSIGN assignment_expression */
#line 141 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MINUS_ASSIGN, 2);
}
#line 1556 "grn_ecmascript.c"
        break;
      case 17: /* assignment_expression ::= lefthand_side_expression SHIFTL_ASSIGN assignment_expression */
#line 144 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTL_ASSIGN, 2);
}
#line 1563 "grn_ecmascript.c"
        break;
      case 18: /* assignment_expression ::= lefthand_side_expression SHIFTR_ASSIGN assignment_expression */
#line 147 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTR_ASSIGN, 2);
}
#line 1570 "grn_ecmascript.c"
        break;
      case 19: /* assignment_expression ::= lefthand_side_expression SHIFTRR_ASSIGN assignment_expression */
#line 150 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTRR_ASSIGN, 2);
}
#line 1577 "grn_ecmascript.c"
        break;
      case 20: /* assignment_expression ::= lefthand_side_expression AND_ASSIGN assignment_expression */
#line 153 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_AND_ASSIGN, 2);
}
#line 1584 "grn_ecmascript.c"
        break;
      case 21: /* assignment_expression ::= lefthand_side_expression XOR_ASSIGN assignment_expression */
#line 156 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_XOR_ASSIGN, 2);
}
#line 1591 "grn_ecmascript.c"
        break;
      case 22: /* assignment_expression ::= lefthand_side_expression OR_ASSIGN assignment_expression */
#line 159 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_OR_ASSIGN, 2);
}
#line 1598 "grn_ecmascript.c"
        break;
      case 23: /* conditional_expression ::= logical_or_expression QUESTION assignment_expression COLON assignment_expression */
#line 164 "grn_ecmascript.lemon"
{
  grn_expr *e = (grn_expr *)efsi->e;
  e->codes[yymsp[-3].minor.yy0].nargs = yymsp[-1].minor.yy0 - yymsp[-3].minor.yy0;
  e->codes[yymsp[-1].minor.yy0].nargs = e->codes_curr - yymsp[-1].minor.yy0 - 1;
}
#line 1607 "grn_ecmascript.c"
        break;
      case 27: /* bitwise_or_expression ::= bitwise_or_expression BITWISE_OR bitwise_xor_expression */
#line 184 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_OR, 2);
}
#line 1614 "grn_ecmascript.c"
        break;
      case 28: /* bitwise_xor_expression ::= bitwise_xor_expression BITWISE_XOR bitwise_and_expression */
#line 189 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_XOR, 2);
}
#line 1621 "grn_ecmascript.c"
        break;
      case 29: /* bitwise_and_expression ::= bitwise_and_expression BITWISE_AND equality_expression */
#line 194 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_AND, 2);
}
#line 1628 "grn_ecmascript.c"
        break;
      case 30: /* equality_expression ::= equality_expression EQUAL relational_expression */
#line 199 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_EQUAL, 2);
}
#line 1635 "grn_ecmascript.c"
        break;
      case 31: /* equality_expression ::= equality_expression NOT_EQUAL relational_expression */
#line 202 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NOT_EQUAL, 2);
}
#line 1642 "grn_ecmascript.c"
        break;
      case 32: /* relational_expression ::= relational_expression LESS shift_expression */
#line 207 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_LESS, 2);
}
#line 1649 "grn_ecmascript.c"
        break;
      case 33: /* relational_expression ::= relational_expression GREATER shift_expression */
#line 210 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_GREATER, 2);
}
#line 1656 "grn_ecmascript.c"
        break;
      case 34: /* relational_expression ::= relational_expression LESS_EQUAL shift_expression */
#line 213 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_LESS_EQUAL, 2);
}
#line 1663 "grn_ecmascript.c"
        break;
      case 35: /* relational_expression ::= relational_expression GREATER_EQUAL shift_expression */
#line 216 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_GREATER_EQUAL, 2);
}
#line 1670 "grn_ecmascript.c"
        break;
      case 36: /* relational_expression ::= relational_expression IN shift_expression */
#line 219 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_IN, 2);
}
#line 1677 "grn_ecmascript.c"
        break;
      case 37: /* relational_expression ::= relational_expression MATCH shift_expression */
      case 89: /* adjust_match_expression ::= IDENTIFIER MATCH STRING */ yytestcase(yyruleno==89);
#line 222 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MATCH, 2);
}
#line 1685 "grn_ecmascript.c"
        break;
      case 38: /* relational_expression ::= relational_expression NEAR shift_expression */
#line 225 "grn_ecmascript.lemon"
{
  {
    int max_interval;
    GRN_INT32_POP(&efsi->max_interval_stack, max_interval);
    grn_expr_append_const_int(efsi->ctx, efsi->e, max_interval,
                              GRN_OP_PUSH, 1);
  }
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NEAR, 3);
}
#line 1698 "grn_ecmascript.c"
        break;
      case 39: /* relational_expression ::= relational_expression NEAR2 shift_expression */
#line 234 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NEAR2, 2);
}
#line 1705 "grn_ecmascript.c"
        break;
      case 40: /* relational_expression ::= relational_expression SIMILAR shift_expression */
#line 237 "grn_ecmascript.lemon"
{
  {
    int similarity_threshold;
    GRN_INT32_POP(&efsi->similarity_threshold_stack, similarity_threshold);
    grn_expr_append_const_int(efsi->ctx, efsi->e, similarity_threshold,
                              GRN_OP_PUSH, 1);
  }
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SIMILAR, 3);
}
#line 1718 "grn_ecmascript.c"
        break;
      case 41: /* relational_expression ::= relational_expression TERM_EXTRACT shift_expression */
#line 246 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_TERM_EXTRACT, 2);
}
#line 1725 "grn_ecmascript.c"
        break;
      case 42: /* relational_expression ::= relational_expression QUORUM shift_expression */
#line 249 "grn_ecmascript.lemon"
{
  {
    int quorum_threshold;
    GRN_INT32_POP(&efsi->quorum_threshold_stack, quorum_threshold);
    grn_expr_append_const_int(efsi->ctx, efsi->e, quorum_threshold,
                              GRN_OP_PUSH, 1);
  }
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_QUORUM, 3);
}
#line 1738 "grn_ecmascript.c"
        break;
      case 43: /* relational_expression ::= relational_expression LCP shift_expression */
#line 258 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_LCP, 2);
}
#line 1745 "grn_ecmascript.c"
        break;
      case 44: /* relational_expression ::= relational_expression PREFIX shift_expression */
#line 261 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PREFIX, 2);
}
#line 1752 "grn_ecmascript.c"
        break;
      case 45: /* relational_expression ::= relational_expression SUFFIX shift_expression */
#line 264 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SUFFIX, 2);
}
#line 1759 "grn_ecmascript.c"
        break;
      case 46: /* relational_expression ::= relational_expression REGEXP shift_expression */
#line 267 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_REGEXP, 2);
}
#line 1766 "grn_ecmascript.c"
        break;
      case 47: /* shift_expression ::= shift_expression SHIFTL additive_expression */
#line 272 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTL, 2);
}
#line 1773 "grn_ecmascript.c"
        break;
      case 48: /* shift_expression ::= shift_expression SHIFTR additive_expression */
#line 275 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTR, 2);
}
#line 1780 "grn_ecmascript.c"
        break;
      case 49: /* shift_expression ::= shift_expression SHIFTRR additive_expression */
#line 278 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SHIFTRR, 2);
}
#line 1787 "grn_ecmascript.c"
        break;
      case 50: /* additive_expression ::= additive_expression PLUS multiplicative_expression */
      case 87: /* adjuster ::= adjuster PLUS adjust_expression */ yytestcase(yyruleno==87);
#line 283 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PLUS, 2);
}
#line 1795 "grn_ecmascript.c"
        break;
      case 51: /* additive_expression ::= additive_expression MINUS multiplicative_expression */
#line 286 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MINUS, 2);
}
#line 1802 "grn_ecmascript.c"
        break;
      case 52: /* multiplicative_expression ::= multiplicative_expression STAR unary_expression */
      case 88: /* adjust_expression ::= adjust_match_expression STAR DECIMAL */ yytestcase(yyruleno==88);
#line 291 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_STAR, 2);
}
#line 1810 "grn_ecmascript.c"
        break;
      case 53: /* multiplicative_expression ::= multiplicative_expression SLASH unary_expression */
#line 294 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_SLASH, 2);
}
#line 1817 "grn_ecmascript.c"
        break;
      case 54: /* multiplicative_expression ::= multiplicative_expression MOD unary_expression */
#line 297 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MOD, 2);
}
#line 1824 "grn_ecmascript.c"
        break;
      case 55: /* unary_expression ::= DELETE unary_expression */
#line 302 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_DELETE, 1);
}
#line 1831 "grn_ecmascript.c"
        break;
      case 56: /* unary_expression ::= INCR unary_expression */
#line 305 "grn_ecmascript.lemon"
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
#line 1852 "grn_ecmascript.c"
        break;
      case 57: /* unary_expression ::= DECR unary_expression */
#line 322 "grn_ecmascript.lemon"
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
#line 1873 "grn_ecmascript.c"
        break;
      case 58: /* unary_expression ::= PLUS unary_expression */
#line 339 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PLUS, 1);
}
#line 1880 "grn_ecmascript.c"
        break;
      case 59: /* unary_expression ::= MINUS unary_expression */
#line 342 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_MINUS, 1);
}
#line 1887 "grn_ecmascript.c"
        break;
      case 60: /* unary_expression ::= NOT unary_expression */
#line 345 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_NOT, 1);
}
#line 1894 "grn_ecmascript.c"
        break;
      case 61: /* unary_expression ::= BITWISE_NOT unary_expression */
#line 348 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_BITWISE_NOT, 1);
}
#line 1901 "grn_ecmascript.c"
        break;
      case 62: /* unary_expression ::= ADJUST unary_expression */
#line 351 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_ADJUST, 1);
}
#line 1908 "grn_ecmascript.c"
        break;
      case 63: /* unary_expression ::= EXACT unary_expression */
#line 354 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_EXACT, 1);
}
#line 1915 "grn_ecmascript.c"
        break;
      case 64: /* unary_expression ::= PARTIAL unary_expression */
#line 357 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_PARTIAL, 1);
}
#line 1922 "grn_ecmascript.c"
        break;
      case 65: /* unary_expression ::= UNSPLIT unary_expression */
#line 360 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_UNSPLIT, 1);
}
#line 1929 "grn_ecmascript.c"
        break;
      case 66: /* postfix_expression ::= lefthand_side_expression INCR */
#line 365 "grn_ecmascript.lemon"
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
#line 1950 "grn_ecmascript.c"
        break;
      case 67: /* postfix_expression ::= lefthand_side_expression DECR */
#line 382 "grn_ecmascript.lemon"
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
#line 1971 "grn_ecmascript.c"
        break;
      case 68: /* call_expression ::= member_expression arguments */
#line 403 "grn_ecmascript.lemon"
{
  grn_expr_append_op(efsi->ctx, efsi->e, GRN_OP_CALL, yymsp[0].minor.yy0);
}
#line 1978 "grn_ecmascript.c"
        break;
      case 69: /* array_literal ::= BRACKETL element_list BRACKETR */
#line 420 "grn_ecmascript.lemon"
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
  assert( yyruleno<sizeof(yyRuleInfo)/sizeof(yyRuleInfo[0]) );
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact <= YY_MAX_SHIFTREDUCE ){
    if( yyact>YY_MAX_SHIFT ){
      yyact += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
    }
    yymsp -= yysize-1;
    yypParser->yytos = yymsp;
    yymsp->stateno = (YYACTIONTYPE)yyact;
    yymsp->major = (YYCODETYPE)yygoto;
    yyTraceShift(yypParser, yyact);
  }else{
    assert( yyact == YY_ACCEPT_ACTION );
    yypParser->yytos -= yysize;
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
  while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
/************ Begin %parse_failure code ***************************************/
/************ End %parse_failure code *****************************************/
  grn_expr_parserARG_STORE; /* Suppress warning about unused %extra_argument variable */
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
  grn_expr_parserARG_FETCH;
#define TOKEN yyminor
/************ Begin %syntax_error code ****************************************/
#line 20 "grn_ecmascript.lemon"

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
#ifndef YYNOERRORRECOVERY
  yypParser->yyerrcnt = -1;
#endif
  assert( yypParser->yytos==yypParser->yystack );
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
/*********** Begin %parse_accept code *****************************************/
/*********** End %parse_accept code *******************************************/
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
  unsigned int yyact;   /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  int yyendofinput;     /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  yypParser = (yyParser*)yyp;
  assert( yypParser->yytos!=0 );
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  yyendofinput = (yymajor==0);
#endif
  grn_expr_parserARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput '%s'\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
    if( yyact <= YY_MAX_SHIFTREDUCE ){
      yy_shift(yypParser,yyact,yymajor,yyminor);
#ifndef YYNOERRORRECOVERY
      yypParser->yyerrcnt--;
#endif
      yymajor = YYNOCODE;
    }else if( yyact <= YY_MAX_REDUCE ){
      yy_reduce(yypParser,yyact-YY_MIN_REDUCE);
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
            && yymx != YYERRORSYMBOL
            && (yyact = yy_find_reduce_action(
                        yypParser->yytos->stateno,
                        YYERRORSYMBOL)) >= YY_MIN_REDUCE
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
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yytos>yypParser->yystack );
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
