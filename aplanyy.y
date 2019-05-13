/*
%code requires {
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void *yyscan_t;
#endif
#include "aplanyy.h"
#include <sys/types.h>
}

%define api.prefix {aplanyy}
*/

%{

/*
#define YYSTYPE APLANYYSTYPE
#define YYLTYPE APLANYYLTYPE
*/

#include "aplanyy.h"
#include "yyutils.h"
#include "aplanyy.tab.h"
#include "aplanyy.lex.h"
#include "abceopcodes.h"
#include <arpa/inet.h>

void aplanyyerror(/*YYLTYPE *yylloc,*/ yyscan_t scanner, struct aplanyy *aplanyy, const char *str)
{
        //fprintf(stderr, "error: %s at line %d col %d\n",str, yylloc->first_line, yylloc->first_column);
        // FIXME we need better location info!
        fprintf(stderr, "aplan error: %s at line %d col %d\n", str, aplanyyget_lineno(scanner), aplanyyget_column(scanner));
}

int aplanyywrap(yyscan_t scanner)
{
        return 1;
}

%}

%pure-parser
%lex-param {yyscan_t scanner}
%parse-param {yyscan_t scanner}
%parse-param {struct aplanyy *aplanyy}
/* %locations */

%union {
  int i;
  double d;
  char *s;
  struct escaped_string str;
  struct {
    int i;
    char *s;
  } both;
  struct {
    uint8_t has_i:1;
    uint8_t has_prio:1;
    int prio;
  } tokenoptstmp;
  struct {
    uint8_t i:1;
    int prio;
  } tokenopts;
}

/*
%destructor { free ($$.str); } STRING_LITERAL
%destructor { free ($$); } VARREF_LITERAL
%destructor { free ($$); } PERCENTLUA_LITERAL
*/


%token <s> PERCENTLUA_LITERAL
%token OPEN_BRACKET
%token CLOSE_BRACKET
%token OPEN_BRACE
%token CLOSE_BRACE
%token OPEN_PAREN
%token CLOSE_PAREN

%token ATTAB

%token NEWLINE

%token EQUALS
/* %token PLUSEQUALS */
/* %token QMEQUALS */
%token COLON
%token COMMA
%token <str> STRING_LITERAL
%token <d> NUMBER
%token <s> VARREF_LITERAL
%token MAYBE_CALL
%token LT
%token GT
%token LE
%token GE
%token AT
%token FUNCTION
%token ENDFUNCTION
%token LOCVAR
%token RECDEP

%token DELAYVAR
%token DELAYEXPR
%token DELAYLISTEXPAND
%token SUFFILTER
%token SUFSUBONE
%token SUFSUB
%token PHONYRULE
%token DISTRULE
%token PATRULE
%token FILEINCLUDE
%token DIRINCLUDE
%token CDEPINCLUDESCURDIR
%token DYNO
%token LEXO
%token IMMO
%token DYN
%token LEX
%token IMM
%token D
%token L
%token I
%token DO
%token LO
%token IO
%token LOC
%token APPEND
%token APPEND_LIST
%token RETURN
%token ADD_RULE
%token RULE_DIST
%token RULE_PHONY
%token RULE_ORDINARY
%token PRINT


%token IF
%token ENDIF
%token WHILE
%token ENDWHILE
%token BREAK
%token CONTINUE

%token DIV MUL ADD SUB SHL SHR NE EQ LOGICAL_AND LOGICAL_OR LOGICAL_NOT MOD BITWISE_AND BITWISE_OR BITWISE_NOT BITWISE_XOR


%token ERROR_TOK

%type<d> value
%type<d> valuelistentry
%type<d> maybeqmequals

%start st

%%

st: aplanrules;

aplanrules:
| aplanrules NEWLINE
| aplanrules assignrule
| aplanrules FUNCTION VARREF_LITERAL OPEN_PAREN maybe_parlist CLOSE_PAREN NEWLINE
{
  size_t funloc = aplanyy->abce.bytecodesz;
  aplanyy_add_fun_sym(aplanyy, $3, 0, funloc);
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_FUN_HEADER);
  aplanyy_add_double(aplanyy, 0);
}
  funlines
  ENDFUNCTION NEWLINE
{
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_PUSH_NIL);
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_RET);
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_FUN_TRAILER);
  aplanyy_add_double(aplanyy, symbol_add(aplanyy, $3, strlen($3)));
  free($3);
}
;

maybe_parlist:
| parlist
;

parlist:
VARREF_LITERAL
{ free($1); abort(); /* not supported yet */}
| parlist COMMA VARREF_LITERAL
{ free($3); abort(); /* not supported yet */}
;

funlines:
  locvarlines
  bodylines
;

locvarlines:
| locvarlines LOCVAR VARREF_LITERAL EQUALS expr NEWLINE
{
  free($3);
}
;

bodylines:
| bodylines statement
;

statement:
  lvalue EQUALS expr NEWLINE
| RETURN expr NEWLINE
{
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_RET);
}
| BREAK NEWLINE
| CONTINUE NEWLINE
| ADD_RULE OPEN_PAREN expr CLOSE_PAREN NEWLINE
| expr NEWLINE
| IF OPEN_PAREN expr CLOSE_PAREN NEWLINE
  bodylines
  ENDIF NEWLINE
| WHILE OPEN_PAREN expr CLOSE_PAREN NEWLINE
  bodylines
  ENDWHILE NEWLINE
;

lvalue:
  varref
| varref maybe_bracketexprlist OPEN_BRACKET expr CLOSE_BRACKET
| DYN OPEN_BRACKET expr CLOSE_BRACKET
| DYN OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist OPEN_BRACKET expr CLOSE_BRACKET
| LEX OPEN_BRACKET expr CLOSE_BRACKET
| LEX OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist OPEN_BRACKET expr CLOSE_BRACKET
| OPEN_PAREN expr CLOSE_PAREN maybe_bracketexprlist OPEN_BRACKET expr CLOSE_BRACKET
;

maybe_bracketexprlist:
| maybe_bracketexprlist OPEN_BRACKET expr CLOSE_BRACKET
;

maybeqmequals: EQUALS {$$ = 0;} /* | QMEQUALS {$$ = 1;} */;

assignrule:
VARREF_LITERAL maybeqmequals
{
  size_t funloc = aplanyy->abce.bytecodesz;
  aplanyy_add_fun_sym(aplanyy, $1, $2, funloc);
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_FUN_HEADER);
  aplanyy_add_double(aplanyy, 0);
}
expr NEWLINE
{
  printf("Assigning to %s\n", $1);
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_RET);
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_FUN_TRAILER);
  aplanyy_add_double(aplanyy, symbol_add(aplanyy, $1, strlen($1)));
  free($1);
}
/*
| VARREF_LITERAL PLUSEQUALS
{
  size_t funloc = aplanyy->bytesz;
  size_t oldloc = aplanyy_add_fun_sym(aplanyy, $1, 0, funloc);
  if (oldloc == (size_t)-1)
  {
    printf("Can't find old symbol function\n");
    YYABORT;
  }
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_FUN_HEADER);
  aplanyy_add_double(aplanyy, 0);
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_PUSH_DBL);
  aplanyy_add_double(aplanyy, oldloc);
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_PUSH_STRINGTAB);
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_CALL_IF_FUN);
  // FIXME what if it's not a list?
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_DUP_NONRECURSIVE);
  //aplanyy_add_byte(aplanyy, ABCE_OPCODE_FUNIFY);
  //aplanyy_add_byte(aplanyy, ABCE_OPCODE_PUSH_DBL);
  //aplanyy_add_double(aplanyy, 0); // arg cnt
  //aplanyy_add_byte(aplanyy, ABCE_OPCODE_CALL);
}
expr NEWLINE
{
  printf("Plus-assigning to %s\n", $1);
  // FIXME what if it's not a list?
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_APPENDALL_MAINTAIN);
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_RET);
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_FUN_TRAILER);
  aplanyy_add_double(aplanyy, symbol_add(aplanyy, $1, strlen($1)));
  free($1);
}
*/
;

value:
  STRING_LITERAL
{
  size_t symid = symbol_add(aplanyy, $1.str, $1.sz);
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_PUSH_DBL);
  aplanyy_add_double(aplanyy, symid);
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
  free($1.str);
  $$ = 0;
}
| varref
{
  $$ = 0;
}
| dict
{
  $$ = 0;
}
| list
{
  $$ = 0;
}
| DELAYVAR OPEN_PAREN varref CLOSE_PAREN
{
  $$ = 0;
}
| DELAYLISTEXPAND OPEN_PAREN expr CLOSE_PAREN
{
  $$ = 1;
}
| DELAYEXPR OPEN_PAREN expr CLOSE_PAREN
{
  $$ = 0;
}
;

varref:
  VARREF_LITERAL
{
  free($1);
}
| DO VARREF_LITERAL
{
  free($2);
}
| LO VARREF_LITERAL
{
  free($2);
}
| IO VARREF_LITERAL
{
  free($2);
}
| D VARREF_LITERAL
{
  free($2);
}
| L VARREF_LITERAL
{
  free($2);
}
| I VARREF_LITERAL
{
  free($2);
}
;

expr: expr11;

expr1:
  expr0
| LOGICAL_NOT expr1
| BITWISE_NOT expr1
| ADD expr1
| SUB expr1
;

expr2:
  expr1
| expr2 MUL expr1
| expr2 DIV expr1
| expr2 MOD expr1
;

expr3:
  expr2
| expr3 ADD expr2
| expr3 SUB expr2
;

expr4:
  expr3
| expr4 SHL expr3
| expr4 SHR expr3
;

expr5:
  expr4
| expr5 LT expr4
| expr5 LE expr4
| expr5 GT expr4
| expr5 GE expr4
;

expr6:
  expr5
| expr6 EQ expr5
| expr6 NE expr5
;

expr7:
  expr6
| expr7 BITWISE_AND expr6
;

expr8:
  expr7
| expr8 BITWISE_XOR expr7
;

expr9:
  expr8
| expr9 BITWISE_OR expr8
;

expr10:
  expr9
| expr10 LOGICAL_AND expr9

expr11:
  expr10
| expr11 LOGICAL_OR expr10


expr0:
  OPEN_PAREN expr CLOSE_PAREN
| OPEN_PAREN expr CLOSE_PAREN OPEN_PAREN maybe_arglist CLOSE_PAREN
| OPEN_PAREN expr CLOSE_PAREN MAYBE_CALL
| dict maybe_bracketexprlist
| list maybe_bracketexprlist
| STRING_LITERAL
{
  free($1.str);
}
| NUMBER
{
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_PUSH_DBL);
  aplanyy_add_double(aplanyy, $1);
}
| lvalue
| lvalue OPEN_PAREN maybe_arglist CLOSE_PAREN
| lvalue MAYBE_CALL
| IMM OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist
| IMM OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist OPEN_PAREN maybe_arglist CLOSE_PAREN
| IMM OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist MAYBE_CALL
| DYNO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist
| DYNO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist OPEN_PAREN maybe_arglist CLOSE_PAREN
| DYNO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist MAYBE_CALL
| LEXO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist
| LEXO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist OPEN_PAREN maybe_arglist CLOSE_PAREN
| LEXO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist MAYBE_CALL
| IMMO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist
| IMMO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist OPEN_PAREN maybe_arglist CLOSE_PAREN
| IMMO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist MAYBE_CALL
| LOC OPEN_BRACKET STRING_LITERAL CLOSE_BRACKET maybe_bracketexprlist
{
  free($3.str);
}
| LOC OPEN_BRACKET STRING_LITERAL CLOSE_BRACKET maybe_bracketexprlist OPEN_PAREN maybe_arglist CLOSE_PAREN
{
  free($3.str);
}
| LOC OPEN_BRACKET STRING_LITERAL CLOSE_BRACKET maybe_bracketexprlist MAYBE_CALL
{
  free($3.str);
}
| SUFSUBONE OPEN_PAREN expr COMMA expr COMMA expr CLOSE_PAREN
| SUFSUB OPEN_PAREN expr COMMA expr COMMA expr CLOSE_PAREN
| SUFFILTER OPEN_PAREN expr COMMA expr CLOSE_PAREN
| APPEND OPEN_PAREN expr COMMA expr CLOSE_PAREN
| APPEND_LIST OPEN_PAREN expr COMMA expr CLOSE_PAREN
| RULE_DIST
| RULE_PHONY
| RULE_ORDINARY
;

maybe_arglist:
| arglist
;

arglist:
expr
| arglist COMMA expr
;

list:
OPEN_BRACKET
{
  aplanyy_add_byte(aplanyy, ABCE_OPCODE_PUSH_NEW_ARRAY);
}
maybe_valuelist
CLOSE_BRACKET
;

dict:
OPEN_BRACE maybe_dictlist CLOSE_BRACE
;

maybe_dictlist:
| dictlist
;

dictlist:
  dictentry
| dictlist COMMA dictentry
;

dictentry:
  STRING_LITERAL COLON value
{
  free($1.str);
}
;

maybe_valuelist:
| valuelist
;

valuelist:
  valuelistentry
{
  if ($1)
  {
    aplanyy_add_byte(aplanyy, ABCE_OPCODE_APPENDALL_MAINTAIN);
  }
  else
  {
    aplanyy_add_byte(aplanyy, ABCE_OPCODE_APPEND_MAINTAIN);
  }
}
| valuelist COMMA
  valuelistentry
{
  if ($3)
  {
    aplanyy_add_byte(aplanyy, ABCE_OPCODE_APPENDALL_MAINTAIN);
  }
  else
  {
    aplanyy_add_byte(aplanyy, ABCE_OPCODE_APPEND_MAINTAIN);
  }
}
;

valuelistentry:
  AT varref
{
  $$ = 0;
}
| value
{
  $$ = $1;
};
