%{

#include "amyplanyy.h"
#include "amyplanyyutils.h"
#include "amyplanyy.tab.h"
#include "amyplanyy.lex.h"
#include "abceopcodes.h"
#include "amyplanlocvarctx.h"
#include "amyplan.h"
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

void add_corresponding_get(struct amyplanyy *amyplanyy, double set)
{
  uint16_t get = get_corresponding_get(set);
  amyplanyy_add_byte(amyplanyy, get);
}

%}

%pure-parser
%lex-param {yyscan_t scanner}
%parse-param {yyscan_t scanner}
%parse-param {struct amyplanyy *amyplanyy}

%union {
  double d;
  char *s;
  struct amyplan_escaped_string str;
}

%token <s> PERCENTLUA_LITERAL
%token OPEN_BRACKET CLOSE_BRACKET OPEN_BRACE CLOSE_BRACE OPEN_PAREN CLOSE_PAREN

%token AT ATTAB NEWLINE TOSTRING TONUMBER

%token EQUALS COLON COMMA
%token <str> STRING_LITERAL
%token <d> NUMBER
%token <s> VARREF_LITERAL
%token MAYBE_CALL
%token LT GT LE GE
%token FUNCTION ENDFUNCTION LOCVAR

%token DYNO LEXO IMMO DYN LEX IMM SCOPE
%token IF ELSE ENDIF WHILE ENDWHILE ONCE ENDONCE BREAK CONTINUE
%token D L I DO LO IO LOC
%token APPEND APPEND_LIST
%token RETURN PRINT

%token STR_FROMCHR STR_LOWER STR_UPPER STR_REVERSE STRCMP STRSTR STRREP
%token STRLISTJOIN STRAPPEND STRSTRIP STRSUB STRGSUB STRSET
%token STRWORD STRWORDCNT STRWORDLIST

%token STDOUT STDERR ERROR DUMP EXIT
%token ABS ACOS ASIN ATAN CEIL COS SIN TAN EXP LOG SQRT
%token DUP_NONRECURSIVE PB_NEW SCOPE_PARENT SCOPE_NEW GETSCOPE_DYN GETSCOPE_LEX

%token DIV MUL ADD SUB SHL SHR NE EQ MOD
%token LOGICAL_AND LOGICAL_OR LOGICAL_NOT
%token BITWISE_AND BITWISE_OR BITWISE_NOT BITWISE_XOR
%token TRUE FALSE NIL ATQM TYPE

%token ERROR_TOK

%type<d> value
%type<d> lvalue
%type<d> arglist
%type<d> valuelistentry
%type<d> maybe_arglist
%type<d> maybe_atqm
%type<d> dynstart
%type<d> scopstart
%type<d> lexstart
%type<d> varref_tail

%start st

%%

st: aplanrules;

aplanrules:
| aplanrules NEWLINE
| aplanrules FUNCTION VARREF_LITERAL
{
  amyplanyy->ctx = amyplan_locvarctx_alloc(NULL, 2, (size_t)-1, (size_t)-1);
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
  amyplanyy_add_double(amyplanyy, amyplan_symbol_add(amyplanyy, $3, strlen($3)));
  free($3);
  amyplan_locvarctx_free(amyplanyy->ctx);
  amyplanyy->ctx = NULL;
}
;

maybe_parlist:
| parlist
;

parlist:
VARREF_LITERAL
{
  amyplan_locvarctx_add_param(amyplanyy->ctx, $1);
  free($1);
}
| parlist COMMA VARREF_LITERAL
{
  amyplan_locvarctx_add_param(amyplanyy->ctx, $3);
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
  amyplan_locvarctx_add(amyplanyy->ctx, $3);
  free($3);
}
;

bodylines:
| bodylines statement
| bodylines NEWLINE
;

statement:
  lvalue EQUALS SUB NEWLINE
{
  if ($1 != ABCE_OPCODE_DICTSET_MAINTAIN)
  {
    printf("Can remove only from dict\n");
    YYABORT;
  }
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_DICTDEL);
}
| lvalue EQUALS expr NEWLINE
{
  if ($1 == ABCE_OPCODE_STRGET)
  {
    printf("Can't assign to string\n");
    YYABORT;
  }
  if ($1 == ABCE_OPCODE_LISTPOP)
  {
    printf("Can't assign to pop query\n");
    YYABORT;
  }
  if ($1 == ABCE_OPCODE_DICTHAS)
  {
    printf("Can't assign to dictionary query\n");
    YYABORT;
  }
  if ($1 == ABCE_OPCODE_SCOPE_HAS)
  {
    printf("Can't assign to scope query\n");
    YYABORT;
  }
  if (   $1 == ABCE_OPCODE_STRLEN || $1 == ABCE_OPCODE_LISTLEN
      || $1 == ABCE_OPCODE_DICTLEN)
  {
    printf("Can't assign to length query (except for PB)\n");
    YYABORT;
  }
  amyplanyy_add_byte(amyplanyy, $1);
  if ($1 == ABCE_OPCODE_DICTSET_MAINTAIN)
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_POP);
  }
}
| RETURN expr NEWLINE
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, amyplan_locvarctx_arg_sz(amyplanyy->ctx)); // arg
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, amyplan_locvarctx_recursive_sz(amyplanyy->ctx)); // loc
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_RETEX2);
}
| BREAK NEWLINE
{
  int64_t loc = amyplan_locvarctx_break(amyplanyy->ctx, 1);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FALSE);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, loc);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_JMP);
}
| CONTINUE NEWLINE
{
  int64_t loc = amyplan_locvarctx_continue(amyplanyy->ctx, 1);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, loc);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_JMP);
}
| BREAK NUMBER NEWLINE
{
  size_t sz = $2;
  int64_t loc;
  if ((double)sz != $2 || sz == 0)
  {
    printf("Break count not positive integer\n");
    YYABORT;
  }
  loc = amyplan_locvarctx_break(amyplanyy->ctx, sz);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FALSE);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, loc);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_JMP);
}
| CONTINUE NUMBER NEWLINE
{
  size_t sz = $2;
  int64_t loc;
  if ((double)sz != $2 || sz == 0)
  {
    printf("Continue count not positive integer\n");
    YYABORT;
  }
  loc = amyplan_locvarctx_continue(amyplanyy->ctx, sz);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, loc);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_JMP);
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
{
  amyplanyy_set_double(amyplanyy, $<d>6, amyplanyy->abce.bytecodesz);
  $<d>$ = $<d>6; // For overwrite by maybe_else
}
  maybe_else
  ENDIF NEWLINE
| WHILE
{
  $<d>$ = amyplanyy->abce.bytecodesz; // startpoint
}
  OPEN_PAREN expr CLOSE_PAREN NEWLINE
{
  struct amyplan_locvarctx *ctx =
    amyplan_locvarctx_alloc(amyplanyy->ctx, 0, amyplanyy->abce.bytecodesz, $<d>2);
  if (ctx == NULL)
  {
    printf("Out of memory\n");
    YYABORT;
  }
  amyplanyy->ctx = ctx;
  $<d>$ = amyplanyy->abce.bytecodesz; // breakpoint
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, -50); // to be overwritten
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_IF_NOT_JMP);
}
  bodylines
  ENDWHILE NEWLINE
{
  struct amyplan_locvarctx *ctx = amyplanyy->ctx->parent;
  free(amyplanyy->ctx);
  amyplanyy->ctx = ctx;
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, $<d>2);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_JMP);
  amyplanyy_set_double(amyplanyy, $<d>7 + 1, amyplanyy->abce.bytecodesz);
}
| ONCE
{
  $<d>$ = amyplanyy->abce.bytecodesz; // startpoint
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_TRUE);
}
  NEWLINE
{
  struct amyplan_locvarctx *ctx =
    amyplan_locvarctx_alloc(amyplanyy->ctx, 0, amyplanyy->abce.bytecodesz, $<d>2);
  if (ctx == NULL)
  {
    printf("Out of memory\n");
    YYABORT;
  }
  amyplanyy->ctx = ctx;
  $<d>$ = amyplanyy->abce.bytecodesz; // breakpoint
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, -50); // to be overwritten
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_IF_NOT_JMP);
}
  bodylines
  ENDONCE NEWLINE
{
  struct amyplan_locvarctx *ctx = amyplanyy->ctx->parent;
  free(amyplanyy->ctx);
  amyplanyy->ctx = ctx;
  amyplanyy_set_double(amyplanyy, $<d>4 + 1, amyplanyy->abce.bytecodesz);
}
| APPEND OPEN_PAREN expr COMMA expr CLOSE_PAREN NEWLINE
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_APPEND_MAINTAIN);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_POP);
}
| APPEND_LIST OPEN_PAREN expr COMMA expr CLOSE_PAREN NEWLINE
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_APPENDALL_MAINTAIN);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_POP);
}
| STDOUT OPEN_PAREN expr CLOSE_PAREN
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, 0);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_OUT);
}
| STDERR OPEN_PAREN expr CLOSE_PAREN
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, 1);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_OUT);
}
| ERROR OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_ERROR); }
| DUMP OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_DUMP); }
| EXIT OPEN_PAREN CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_EXIT); }
;

maybe_else:
| ELSE NEWLINE
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  $<d>$ = amyplanyy->abce.bytecodesz;
  amyplanyy_add_double(amyplanyy, -50); // to be overwritten
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_JMP);
  amyplanyy_set_double(amyplanyy, $<d>0, amyplanyy->abce.bytecodesz); // Overwrite
}
bodylines
{
  amyplanyy_set_double(amyplanyy, $<d>3, amyplanyy->abce.bytecodesz);
}
;

varref_tail:
  OPEN_BRACKET expr CLOSE_BRACKET
{
  $$ = ABCE_OPCODE_LISTSET;
}
| OPEN_BRACKET SUB CLOSE_BRACKET
{
  $$ = ABCE_OPCODE_LISTPOP; // This is special. Can't assign to pop query.
}
| OPEN_BRACKET CLOSE_BRACKET
{
  $$ = ABCE_OPCODE_LISTLEN; // This is special. Can't assign to length query.
}
| OPEN_BRACE expr CLOSE_BRACE
{
  $$ = ABCE_OPCODE_DICTSET_MAINTAIN; // FIXME remember to pop!
}
| OPEN_BRACE CLOSE_BRACE
{
  $$ = ABCE_OPCODE_DICTLEN; // This is special. Can't assign to length query.
}
| OPEN_BRACE ATQM expr CLOSE_BRACE
{
  $$ = ABCE_OPCODE_DICTHAS; // This is special. Can't assign to "has" query.
}
| OPEN_BRACKET AT expr CLOSE_BRACKET
{
  $$ = ABCE_OPCODE_STRGET; // This is special. Can't assign to string.
}
| OPEN_BRACKET AT CLOSE_BRACKET
{
  $$ = ABCE_OPCODE_STRLEN; // This is special. Can't assign to length query.
}
| OPEN_BRACE AT expr CLOSE_BRACE
{
  $$ = ABCE_OPCODE_PBSET; // FIXME needs transfer size of operation
}
| OPEN_BRACE AT CLOSE_BRACE
{
  $$ = ABCE_OPCODE_PBSETLEN; // This is very special: CAN assign to length query
}
;

lvalue:
  varref
{
  $$ = ABCE_OPCODE_SET_STACK;
}
| varref
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_STACK);
}
  maybe_bracketexprlist varref_tail
{
  $$ = $4;
}
| dynstart
{
  $$ = $1;
}
| dynstart
{
  amyplanyy_add_byte(amyplanyy,
    ($1 == ABCE_OPCODE_SCOPEVAR_SET) ? ABCE_OPCODE_SCOPEVAR : $1);
}
  maybe_bracketexprlist varref_tail
{
  $$ = $4;
}
| scopstart
{
  $$ = $1;
}
| scopstart
{
  amyplanyy_add_byte(amyplanyy,
    ($1 == ABCE_OPCODE_SCOPEVAR_SET) ? ABCE_OPCODE_SCOPEVAR : $1);
}
  maybe_bracketexprlist varref_tail
{
  $$ = $4;
}
| lexstart
{
  $$ = $1;
}
| lexstart
{
  amyplanyy_add_byte(amyplanyy,
    ($1 == ABCE_OPCODE_SCOPEVAR_SET) ? ABCE_OPCODE_SCOPEVAR : $1);
}
  maybe_bracketexprlist varref_tail
{
  $$ = $4;
}
| OPEN_PAREN expr CLOSE_PAREN maybe_bracketexprlist varref_tail
{
  $$ = $5;
}
;

maybe_atqm:
{
  $$ = ABCE_OPCODE_SCOPEVAR_SET;
}
| ATQM
{
  $$ = ABCE_OPCODE_SCOPE_HAS;
}
;

lexstart:
  LEX
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, amyplanyy->abce.dynscope.u.area->u.sc.locidx);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
}
  OPEN_BRACKET maybe_atqm expr CLOSE_BRACKET
{
  $$ = $4;
}
;

dynstart:
  DYN
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_GETSCOPE_DYN);
}
  OPEN_BRACKET maybe_atqm expr CLOSE_BRACKET
{
  $$ = $4;
}
;

scopstart:
  SCOPE OPEN_BRACKET expr COMMA maybe_atqm expr CLOSE_BRACKET
{
  $$ = $5;
}
;

maybe_bracketexprlist:
| maybe_bracketexprlist varref_tail
{
  if ($2 == ABCE_OPCODE_LISTSET)
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LISTGET);
  }
  else
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_DICTGET);
  }
}
;

value:
  AT expr
{
  $$ = 1;
}
| expr
{
  $$ = 0;
}
;

varref:
  VARREF_LITERAL
{
  int64_t locvar;
  locvar = amyplan_locvarctx_search_rec(amyplanyy->ctx, $1);
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
  printf("DO not supported yet\n");
  abort();
}
| LO VARREF_LITERAL
{
  free($2);
  printf("LO not supported yet\n");
  abort();
}
| IO VARREF_LITERAL
{
  free($2);
  printf("IO not supported yet\n");
  abort();
}
| D VARREF_LITERAL
{
  free($2);
  printf("D not supported yet\n");
  abort();
}
| L VARREF_LITERAL
{
  free($2);
  printf("L not supported yet\n");
  abort();
}
| I VARREF_LITERAL
{
  free($2);
  printf("I not supported yet\n");
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
| TRUE { amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_TRUE); }
| TYPE OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_TYPE); }
| FALSE { amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FALSE); }
| NIL { amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_NIL); }
| STR_FROMCHR OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STR_FROMCHR); }
| STR_LOWER OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STR_LOWER); }
| STR_UPPER OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STR_UPPER); }
| STR_REVERSE OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STR_REVERSE); }
| STRCMP OPEN_PAREN expr COMMA expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STR_CMP); }
| STRSTR OPEN_PAREN expr COMMA expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRSTR); }
| STRREP OPEN_PAREN expr COMMA expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRREP); }
| STRLISTJOIN OPEN_PAREN expr COMMA expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRLISTJOIN); }
| STRAPPEND OPEN_PAREN expr COMMA expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRAPPEND); }
| STRSTRIP OPEN_PAREN expr COMMA expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRSTRIP); }
| STRSUB OPEN_PAREN expr COMMA expr COMMA expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRSUB); }
| STRGSUB OPEN_PAREN expr COMMA expr COMMA expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRGSUB); }
| STRSET OPEN_PAREN expr COMMA expr COMMA expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRSET); }
| STRWORD OPEN_PAREN expr COMMA expr COMMA expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRWORD); }
| STRWORDCNT OPEN_PAREN expr COMMA expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRWORDCNT); }
| STRWORDLIST OPEN_PAREN expr COMMA expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRWORDCNT); }
| ABS OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_ABS); }
| ACOS OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_ACOS); }
| ASIN OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_ASIN); }
| ATAN OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_ATAN); }
| CEIL OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CEIL); }
| COS OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_COS); }
| SIN OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_SIN); }
| TAN OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_TAN); }
| EXP OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_EXP); }
| LOG OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LOG); }
| SQRT OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_SQRT); }
| DUP_NONRECURSIVE OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_DUP_NONRECURSIVE); }
| PB_NEW OPEN_PAREN CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_NEW_PB); }
| TOSTRING OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_TOSTRING); }
| TONUMBER OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_TONUMBER); }
| SCOPE_PARENT OPEN_PAREN expr CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_SCOPE_PARENT); }
| SCOPE_NEW OPEN_PAREN expr COMMA expr CLOSE_PAREN
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_SCOPE_NEW);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
}
| GETSCOPE_DYN OPEN_PAREN CLOSE_PAREN
{ amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_GETSCOPE_DYN); }
| GETSCOPE_LEX OPEN_PAREN CLOSE_PAREN
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, amyplanyy->abce.dynscope.u.area->u.sc.locidx);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
}
| lvalue
{
  add_corresponding_get(amyplanyy, $1);
}
| lvalue
{
  add_corresponding_get(amyplanyy, $1);
}
  OPEN_PAREN maybe_arglist CLOSE_PAREN
{
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  amyplanyy_add_double(amyplanyy, $4);
  amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CALL);
}
| lvalue MAYBE_CALL
{
  add_corresponding_get(amyplanyy, $1);
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
maybe_valuelist CLOSE_BRACKET
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
  value COLON value
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
| valuelist COMMA valuelistentry
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
  value
{
  $$ = $1;
};
