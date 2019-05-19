%option nounput noinput
%option prefix="amyplanyy"

%{
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include "amyplanyyutils.h"
#include "amyplanyy.tab.h"

/*
#define YYSTYPE APLANYYSTYPE
#define YYLTYPE APLANYYLTYPE
*/

/*
#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno; \
  yylloc->first_column = yycolumn; yylloc->last_column = yycolumn+yyleng-1; \
  yycolumn += yyleng;
*/
#define YY_USER_ACTION yycolumn += yyleng;

// @listexpand return LISTEXPAND;

%}

%option reentrant bison-bridge
%x PERCENTLUA

%%

%{
struct amyplan_csnippet luaSnippet = {
  .data = NULL,
  .len = 0,
  .capacity = 0,
};
%}

@         return AT;
@print    return PRINT;
@function return FUNCTION;
@endfunction return ENDFUNCTION;
@locvar   return LOCVAR;
@D     return D;
@L     return L;
@I     return I;
@DO     return DO;
@LO     return LO;
@IO     return IO;
@dyn   return DYN;
@lex   return LEX;
@imm   return IMM;
@dyno   return DYNO;
@lexo   return LEXO;
@immo   return IMMO;
@loc   return LOC;
@if   return IF;
@else   return ELSE;
@endif   return ENDIF;
@while return WHILE;
@endwhile return ENDWHILE;
@break return BREAK;
@continue return CONTINUE;
@return  return RETURN;
@append   return APPEND;
@appendList   return APPEND_LIST;
:         return COLON;
,         return COMMA;
=         return EQUALS;
&& return LOGICAL_AND;
\|\| return LOGICAL_OR;
!= return NE;
== return EQ;
\<\< return SHL;
\>\> return SHR;
\<        return LT;
\>        return GT;
[(]       return OPEN_PAREN;
[)]       return CLOSE_PAREN;
[[]       return OPEN_BRACKET;
[]]       return CLOSE_BRACKET;
[{]       return OPEN_BRACE;
[}]       return CLOSE_BRACE;
\/ return DIV;
\* return MUL;
\+ return ADD;
- return SUB;
& return BITWISE_AND;
\| return BITWISE_OR;
~ return BITWISE_NOT;
\^ return BITWISE_XOR;
! return LOGICAL_NOT;
% return MOD;
\>= return GE;
\<= return LE;
\<\> return MAYBE_CALL;


[$][A-Za-z0-9_.]+ {
  yylval->s = strdup(yytext+1);
  return VARREF_LITERAL;
}
[$][(][A-Za-z0-9_.]+[)] {
  yylval->s = strdup(yytext+2);
  yylval->s[strlen(yylval->s)-1] = '\0';
  return VARREF_LITERAL;
}
[$][{][A-Za-z0-9_.]+[}] {
  yylval->s = strdup(yytext+2);
  yylval->s[strlen(yylval->s)-1] = '\0';
  return VARREF_LITERAL;
}
[-+]?[0-9]+(.[0-9]+)?([eE][-+]?[0-9]+)? {
  yylval->d = atof(yytext);
  return NUMBER;
}

\"([^\\\"]|\\.)*\"  yylval->str=amyplanyy_escape_string(yytext); return STRING_LITERAL;
\'([^\\\']|\\.)*\'  yylval->str=amyplanyy_escape_string_single(yytext); return STRING_LITERAL;
#[^\n]*  /* ignore */;

"%lua{"             { BEGIN(PERCENTLUA); luaSnippet.capacity = 0; luaSnippet.len = 0; luaSnippet.data = NULL; }
<PERCENTLUA>%\}  {
                     BEGIN(INITIAL);
                     yylval->s = luaSnippet.data;
                     luaSnippet.data = NULL;
                     luaSnippet.capacity = 0;
                     luaSnippet.len = 0;
                     return PERCENTLUA_LITERAL;
                 }
<PERCENTLUA>.             { amyplan_csadd(&luaSnippet, yytext[0]); }
<PERCENTLUA>"\n"          { amyplan_csadd(&luaSnippet, yytext[0]); ++yylineno; yycolumn=1; }

\n                                { ++yylineno; yycolumn=1; return NEWLINE; }
\\\n                              { ++yylineno; yycolumn=1; }
[ \t]                            /* ignore whitespaces */;
.            return ERROR_TOK;


%%
