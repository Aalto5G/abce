/*
%code requires {
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void *yyscan_t;
#endif
#include "amyplanyy.h"
#include <sys/types.h>
}

%define api.prefix {amyplanyy}
*/

%{

/*
#define YYSTYPE APLANYYSTYPE
#define YYLTYPE APLANYYLTYPE
*/

#include "amyplanyy.h"
#include "yyutils.h"
#include "amyplanyy.tab.h"
#include "amyplanyy.lex.h"
#include "abceopcodes.h"
#include "locvarctx.h"
#include <arpa/inet.h>

void amyplanyyerror(/*YYLTYPE *yylloc,*/ yyscan_t scanner, struct amyplanyy *amyplanyy, const char *str)
{
        //fprintf(stderr, "error: %s at line %d col %d\n",str, yylloc->first_line, yylloc->first_column);
        // FIXME we need better location info!
        fprintf(stderr, "aplan error: %s at line %d col %d\n", str, amyplanyyget_lineno(scanner), amyplanyyget_column(scanner));
}

int amyplanyywrap(yyscan_t scanner)
{
        return 1;
}

%}

%pure-parser
%lex-param {yyscan_t scanner}
%parse-param {yyscan_t scanner}
%parse-param {struct amyplanyy *amyplanyy}
/* %locations */

%union {
  int i;
  struct {
    double d1;
    double d2;
  } d2;
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

/*
%token DELAYVAR
%token DELAYEXPR
%token DELAYLISTEXPAND
*/
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
%type<d> lvalue
%type<d> arglist
%type<d> valuelistentry
%type<d> maybe_arglist
%type<d> maybeqmequals

%start st

%%

st: aplanrules;

aplanrules:
| aplanrules NEWLINE
| aplanrules assignrule
| aplanrules FUNCTION VARREF_LITERAL
{
  amyplanyy->ctx = abce_locvarctx_alloc(NULL, 2, (size_t)-1, (size_t)-1);
}
OPEN_PAREN maybe_parlist CLOSE_PAREN NEWLINE
{
  size_t funloc = amyplanyy->abce.bytecodesz;
  amyplanyy_add_fun_sym(amyplanyy, $3, 0, funloc);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_FUN_HEADER);
  amyplanyy_add_double(amyplanyy, amyplanyy->ctx->args);
}
  funlines
  ENDFUNCTION NEWLINE
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_NIL); // retval
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, amyplanyy->ctx->args); // argcnt
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, amyplanyy->ctx->sz - amyplanyy->ctx->args); // locvarcnt
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_RETEX2);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_FUN_TRAILER);
  amyplanyy_add_double(amyplanyy, symbol_add(amyplanyy, $3, strlen($3)));
  free($3);
  abce_locvarctx_free(amyplanyy->ctx);
  amyplanyy->ctx = NULL;
}
;

maybe_parlist:
| parlist
;

parlist:
VARREF_LITERAL
{
  abce_locvarctx_add_param(amyplanyy->ctx, $1);
  free($1);
}
| parlist COMMA VARREF_LITERAL
{
  abce_locvarctx_add_param(amyplanyy->ctx, $3);
  free($3);
}
;

funlines:
  locvarlines
  bodylines
;

locvarlines:
| locvarlines LOCVAR VARREF_LITERAL EQUALS expr NEWLINE
{
  abce_locvarctx_add(amyplanyy->ctx, $3);
  free($3);
}
;

bodylines:
| bodylines statement
;

statement:
  lvalue EQUALS expr NEWLINE
{
  amyplanyy_add_byte(amyplanyy, $1);
  if ($1 == ABCE_OPCODE_DICTSET_MAINTAIN)
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_POP);
  }
}
| RETURN expr NEWLINE
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_byte(amyplanyy, abce_locvarctx_arg_sz(amyplanyy->ctx)); // arg
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_byte(amyplanyy, abce_locvarctx_recursive_sz(amyplanyy->ctx)); // loc
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_RETEX2);
}
| BREAK NEWLINE
{
  abort(); // not supported yet, FIXME!
}
| CONTINUE NEWLINE
{
  abort(); // not supported yet, FIXME!
}
| expr NEWLINE
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_POP); // called for side effects only
}
| IF OPEN_PAREN expr CLOSE_PAREN NEWLINE
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  $<d>$ = amyplanyy->abce.bytecodesz;
  amyplanyy_add_double(amyplanyy, -50); // to be overwritten
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_IF_NOT_JMP);
}
  bodylines
  ENDIF NEWLINE
{
  amyplanyy_set_double(amyplanyy, $<d>6, amyplanyy->abce.bytecodesz);
}
| WHILE OPEN_PAREN expr CLOSE_PAREN NEWLINE
{
  $<d>$ = amyplanyy->abce.bytecodesz;
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, -50); // to be overwritten
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_IF_NOT_JMP);
}
  bodylines
  ENDWHILE NEWLINE
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, $<d>6);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_JMP);
  amyplanyy_set_double(amyplanyy, $<d>6 + 1, amyplanyy->abce.bytecodesz);
}
;

lvalue:
  varref
{
  $$ = ABCE_OPCODE_SET_STACK;
}
| varref maybe_bracketexprlist OPEN_BRACKET expr CLOSE_BRACKET
{
  $$ = ABCE_OPCODE_LISTSET;
}
| DYN OPEN_BRACKET expr CLOSE_BRACKET
{
  abort(); // FIXME not supported yet
}
| DYN OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist OPEN_BRACKET expr CLOSE_BRACKET
{
  abort(); // FIXME not supported yet
}
| LEX OPEN_BRACKET expr CLOSE_BRACKET
{
  abort(); // FIXME not supported yet
}
| LEX OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist OPEN_BRACKET expr CLOSE_BRACKET
{
  abort(); // FIXME not supported yet
}
| OPEN_PAREN expr CLOSE_PAREN maybe_bracketexprlist OPEN_BRACKET expr CLOSE_BRACKET
{
  $$ = ABCE_OPCODE_LISTSET;
}
;

maybe_bracketexprlist:
| maybe_bracketexprlist OPEN_BRACKET expr CLOSE_BRACKET
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LISTGET);
}
;

maybeqmequals: EQUALS {$$ = 0;} /* | QMEQUALS {$$ = 1;} */;

assignrule:
VARREF_LITERAL maybeqmequals
{
  size_t funloc = amyplanyy->abce.bytecodesz;
  amyplanyy_add_fun_sym(amyplanyy, $1, $2, funloc);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_FUN_HEADER);
  amyplanyy_add_double(amyplanyy, 0);
}
expr NEWLINE
{
  printf("Assigning to %s\n", $1);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_RET);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_FUN_TRAILER);
  amyplanyy_add_double(amyplanyy, symbol_add(amyplanyy, $1, strlen($1)));
  free($1);
}
/*
| VARREF_LITERAL PLUSEQUALS
{
  size_t funloc = amyplanyy->bytesz;
  size_t oldloc = amyplanyy_add_fun_sym(amyplanyy, $1, 0, funloc);
  if (oldloc == (size_t)-1)
  {
    printf("Can't find old symbol function\n");
    YYABORT;
  }
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_FUN_HEADER);
  amyplanyy_add_double(amyplanyy, 0);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, oldloc);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_STRINGTAB);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CALL_IF_FUN);
  // FIXME what if it's not a list?
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_DUP_NONRECURSIVE);
  //amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_FUNIFY);
  //amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  //amyplanyy_add_double(amyplanyy, 0); // arg cnt
  //amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CALL);
}
expr NEWLINE
{
  printf("Plus-assigning to %s\n", $1);
  // FIXME what if it's not a list?
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_APPENDALL_MAINTAIN);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_RET);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_FUN_TRAILER);
  amyplanyy_add_double(amyplanyy, symbol_add(amyplanyy, $1, strlen($1)));
  free($1);
}
*/
;

value:
/*
  STRING_LITERAL
{
  int64_t symid = abce_cache_add_str(&amyplanyy->abce, $1.str, $1.sz);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, symid);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
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
|*/ AT /* DELAYLISTEXPAND */ /* OPEN_PAREN */ expr /* CLOSE_PAREN */
{
  $$ = 1;
}
| /* DELAYEXPR */ /* OPEN_PAREN */ expr /* CLOSE_PAREN */ /* FIXME! */
{
  $$ = 0;
}
;

varref:
  VARREF_LITERAL
{
  int64_t locvar;
  locvar = abce_locvarctx_search_rec(amyplanyy->ctx, $1);
  if (locvar >= 0)
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, locvar);
  }
  else
  {
    printf("var %s not found\n", $1);
    abort();
  }
  free($1);
}
| DO VARREF_LITERAL
{
  free($2);
  abort();
}
| LO VARREF_LITERAL
{
  free($2);
  abort();
}
| IO VARREF_LITERAL
{
  free($2);
  abort();
}
| D VARREF_LITERAL
{
  free($2);
  abort();
}
| L VARREF_LITERAL
{
  free($2);
  abort();
}
| I VARREF_LITERAL
{
  free($2);
  abort();
}
;

expr: expr11;

expr1:
  expr0
| LOGICAL_NOT expr1
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LOGICAL_NOT);
}
| BITWISE_NOT expr1
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_BITWISE_NOT);
}
| ADD expr1
| SUB expr1
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_UNARY_MINUS);
}
;

expr2:
  expr1
| expr2 MUL expr1
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_MUL);
}
| expr2 DIV expr1
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_DIV);
}
| expr2 MOD expr1
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_MOD);
}
;

expr3:
  expr2
| expr3 ADD expr2
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_ADD);
}
| expr3 SUB expr2
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_SUB);
}
;

expr4:
  expr3
| expr4 SHL expr3
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_SHL);
}
| expr4 SHR expr3
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_SHR);
}
;

expr5:
  expr4
| expr5 LT expr4
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LT);
}
| expr5 LE expr4
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LE);
}
| expr5 GT expr4
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_GT);
}
| expr5 GE expr4
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_GE);
}
;

expr6:
  expr5
| expr6 EQ expr5
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_EQ);
}
| expr6 NE expr5
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_NE);
}
;

expr7:
  expr6
| expr7 BITWISE_AND expr6
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_BITWISE_AND);
}
;

expr8:
  expr7
| expr8 BITWISE_XOR expr7
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_BITWISE_XOR);
}
;

expr9:
  expr8
| expr9 BITWISE_OR expr8
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_BITWISE_OR);
}
;

expr10:
  expr9
| expr10 LOGICAL_AND expr9
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LOGICAL_AND);
}
;

expr11:
  expr10
| expr11 LOGICAL_OR expr10
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LOGICAL_OR);
}
;


expr0:
  OPEN_PAREN expr CLOSE_PAREN
| OPEN_PAREN expr CLOSE_PAREN OPEN_PAREN maybe_arglist CLOSE_PAREN
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, $5);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CALL);
}
| OPEN_PAREN expr CLOSE_PAREN MAYBE_CALL
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CALL_IF_FUN);
}
| dict maybe_bracketexprlist
| list maybe_bracketexprlist
| STRING_LITERAL
{
  int64_t idx = abce_cache_add_str(&amyplanyy->abce, $1.str, $1.sz);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, idx);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
  free($1.str);
}
| NUMBER
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, $1);
}
| lvalue
{
  switch ((int)$1)
  {
    case ABCE_OPCODE_SET_STACK:
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_STACK);
      break;
    case ABCE_OPCODE_LISTSET:
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LISTGET);
      break;
    case ABCE_OPCODE_DICTSET_MAINTAIN:
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_DICTGET);
      break;
    case ABCE_OPCODE_PBSET:
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PBGET);
      break;
    default:
      abort();
  }
}
| lvalue OPEN_PAREN maybe_arglist CLOSE_PAREN
{
  switch ((int)$1)
  {
    case ABCE_OPCODE_SET_STACK:
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_STACK);
      break;
    case ABCE_OPCODE_LISTSET:
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LISTGET);
      break;
    case ABCE_OPCODE_DICTSET_MAINTAIN:
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_DICTGET);
      break;
    case ABCE_OPCODE_PBSET:
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PBGET);
      break;
    default:
      abort();
  }
  // push new_ip, argcnt
  // FIXME new_ip is before args on stack
  abort();
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, $3);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CALL);
}
| lvalue MAYBE_CALL
{
  switch ((int)$1)
  {
    case ABCE_OPCODE_SET_STACK:
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_STACK);
      break;
    case ABCE_OPCODE_LISTSET:
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LISTGET);
      break;
    case ABCE_OPCODE_DICTSET_MAINTAIN:
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_DICTGET);
      break;
    case ABCE_OPCODE_PBSET:
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PBGET);
      break;
    default:
      abort();
  }
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CALL_IF_FUN);
}
| IMM OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist
{
  abort();
}
| IMM OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist OPEN_PAREN maybe_arglist CLOSE_PAREN
{
  abort();
}
| IMM OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist MAYBE_CALL
{
  abort();
}
| DYNO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist
{
  abort();
}
| DYNO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist OPEN_PAREN maybe_arglist CLOSE_PAREN
{
  abort();
}
| DYNO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist MAYBE_CALL
{
  abort();
}
| LEXO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist
{
  abort();
}
| LEXO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist OPEN_PAREN maybe_arglist CLOSE_PAREN
{
  abort();
}
| LEXO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist MAYBE_CALL
{
  abort();
}
| IMMO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist
{
  abort();
}
| IMMO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist OPEN_PAREN maybe_arglist CLOSE_PAREN
{
  abort();
}
| IMMO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist MAYBE_CALL
{
  abort();
}
| LOC OPEN_BRACKET STRING_LITERAL CLOSE_BRACKET maybe_bracketexprlist
{
  free($3.str);
  abort();
}
| LOC OPEN_BRACKET STRING_LITERAL CLOSE_BRACKET maybe_bracketexprlist OPEN_PAREN maybe_arglist CLOSE_PAREN
{
  free($3.str);
  abort();
}
| LOC OPEN_BRACKET STRING_LITERAL CLOSE_BRACKET maybe_bracketexprlist MAYBE_CALL
{
  free($3.str);
  abort();
}
| APPEND OPEN_PAREN expr COMMA expr CLOSE_PAREN
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_APPEND_MAINTAIN);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_POP);
}
| APPEND_LIST OPEN_PAREN expr COMMA expr CLOSE_PAREN
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_APPENDALL_MAINTAIN);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_POP);
}
;

maybe_arglist:
{
  $$ = 0;
}
| arglist
{
  $$ = $1;
}
;

arglist:
expr
{
  $$ = 1;
}
| arglist COMMA expr
{
  $$ = $1 + 1;
}
;

list:
OPEN_BRACKET
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_NEW_ARRAY);
}
maybe_valuelist
CLOSE_BRACKET
;

dict:
OPEN_BRACE
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_NEW_DICT);
}
maybe_dictlist CLOSE_BRACE
;

maybe_dictlist:
| dictlist
;

dictlist:
  dictentry
| dictlist COMMA dictentry
;

dictentry:
  value
/*
STRING_LITERAL
{
  int64_t idx = abce_cache_add_str(&amyplanyy->abce, $1.str, $1.sz);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, idx);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
  free($1.str);
}
*/
COLON value
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_DICTSET_MAINTAIN);
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
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_APPENDALL_MAINTAIN);
  }
  else
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_APPEND_MAINTAIN);
  }
}
| valuelist COMMA
  valuelistentry
{
  if ($3)
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_APPENDALL_MAINTAIN);
  }
  else
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_APPEND_MAINTAIN);
  }
}
;

valuelistentry:
/*
  AT varref
{
  $$ = 0;
}
|
*/
value
{
  $$ = $1;
};
