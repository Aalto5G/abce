%option nounput noinput
%option prefix="amyplanyy"

%{
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "amyplanyyutils.h"
#include "amyplanyy.tab.h"

/*
#define YYSTYPE AMYPLANYYSTYPE
#define YYLTYPE AMYPLANYYLTYPE
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

@tostring return TOSTRING;
@tonumber return TONUMBER;
@stdout   return STDOUT; /* FIXME make these true streams */
@stderr   return STDERR;
@error    return ERROR;
@dump     return DUMP;
@exit     return EXIT;
@abs      return ABS;
@acos     return ACOS;
@asin     return ASIN;
@atan     return ATAN;
@ceil     return CEIL;
@floor    return FLOOR;
@trunc    return TRUNC;
@round    return ROUND;
@cos      return COS;
@sin      return SIN;
@tan      return TAN;
@exp      return EXP;
@log      return LOG;
@sqrt     return SQRT;
@getenv   return GETENV;
@chomp    return CHOMP;
@dupnonrec return DUP_NONRECURSIVE;
@pbnew    return PB_NEW;
@fpclassify return FP_CLASSIFY;
@be8      return EIGHT;
@le8      return EIGHT;
@be16     return SIXTEENBE;
@be32     return THIRTYTWOBE;
@le16     return SIXTEENLE;
@le32     return THIRTYTWOLE;
@str2pb   return STR2PB;
@pb2str   return PB2STR;
@scopeparent return SCOPE_PARENT;
@scopenew return SCOPE_NEW;
@scope    return SCOPE;
@splice   return SPLICE;
@getscopedyn return GETSCOPE_DYN;
@getscopelex return GETSCOPE_LEX;
@beginscope return BEGINSCOPE;
@beginholeyscope return BEGINHOLEYSCOPE;
@endscope return ENDSCOPE;
@for      return FOR;
@fordict  return FORDICT;
@fordictprev  return FORDICTPREV;
@endfor   return ENDFOR;
@fopen    return FOPEN;
@fclose   return FCLOSE;
@fread    return FREAD;
@fseek    return FSEEK;
@fgetdelim return FGETDELIM;
@fflush   return FFLUSH;
@fwrite   return FWRITE;
@jsonenc  return JSONENC;
@jsondec  return JSONDEC;

@         return AT;
@type     return TYPE;
@[?]      return ATQM;
@true     return TRUE;
@nil      return NIL;
@false    return FALSE;
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
@DP     return DP;
@LP     return LP;
@IP     return IP;
@DPO     return DPO;
@LPO     return LPO;
@IPO     return IPO;
@SC      return SC;
@SCO     return SCO;
@dyn   return DYN;
@lex   return LEX;
@imm   return IMM;
@dyno   return DYNO;
@lexo   return LEXO;
@immo   return IMMO;
@loc   return LOC;
@if   return IF;
@else   return ELSE;
@elseif   return ELSEIF;
@endif   return ENDIF;
@while return WHILE;
@endwhile return ENDWHILE;
@once return ONCE;
@endonce return ENDONCE;
@break return BREAK;
@continue return CONTINUE;
@return  return RETURN;
@append   return APPEND;
@dictnext return DICTNEXT;
@dictprev return DICTPREV;
@strstr return STRSTR;
@strrep return STRREP;
@strlistjoin return STRLISTJOIN;
@strappend return STRAPPEND;
@strstrip return STRSTRIP;
@strsub return STRSUB;
@strgsub return STRGSUB;
@strset return STRSET;
@strword return STRWORD;
@strwordcnt return STRWORDCNT;
@strwordlist return STRWORDLIST;
@strcmp return STRCMP;
@strreverse return STR_REVERSE;
@strupper return STR_UPPER;
@strlower return STR_LOWER;
@strfromchr return STR_FROMCHR;
@appendlist   return APPEND_LIST;
@dynluacall  return DYNLUACALL;
@lexluacall  return LEXLUACALL;
@gettime  return GETTIME;
@localtime  return LOCALTIME;
@gmtime  return GMTIME;
[+]=      return PLUSEQUALS;
[?]=      return QMEQUALS;
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
[.] return PERIOD;


[$][A-Za-z0-9_]+ {
  yylval->s = strdup(yytext+1);
  return VARREF_LITERAL;
}
[$][(][A-Za-z0-9_]+[)] {
  yylval->s = strdup(yytext+2);
  yylval->s[strlen(yylval->s)-1] = '\0';
  return VARREF_LITERAL;
}
[$][{][A-Za-z0-9_]+[}] {
  yylval->s = strdup(yytext+2);
  yylval->s[strlen(yylval->s)-1] = '\0';
  return VARREF_LITERAL;
}
[0-9]+([.][0-9]+)?([eE][-+]?[0-9]+)? {
  // Original regexp was: [-+]?[0-9]+([.][0-9]+)?([eE][-+]?[0-9]+)?
  // The sign from original regexp was removed to allow $a=$a+1 to work
  yylval->d = atof(yytext);
  return NUMBER;
}

\"([^\\\"]|\\.)*\"  yylval->str=amyplanyy_escape_string(yytext, '"'); return STRING_LITERAL;
\'([^\\\']|\\.)*\'  yylval->str=amyplanyy_escape_string(yytext, '\''); return STRING_LITERAL;
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
