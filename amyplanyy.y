%{

#include "amyplanyy.h"
#include "amyplanyyutils.h"
#include "amyplanyy.tab.h"
#include "amyplanyy.lex.h"
#include "abceopcodes.h"
#include "abcescopes.h"
#include "amyplanlocvarctx.h"
#include "amyplan.h"

void amyplanyyerror(/*YYLTYPE *yylloc,*/ yyscan_t scanner, struct amyplanyy *amyplanyy, const char *str)
{
        //fprintf(stderr, "error: %s at line %d col %d\n",str, yylloc->first_line, yylloc->first_column);
        // FIXME we need better location info!
        fprintf(stderr, "amyplan error: %s at line %d col %d\n", str, amyplanyyget_lineno(scanner), amyplanyyget_column(scanner));
}

int amyplanyywrap(yyscan_t scanner)
{
        return 1;
}

static inline int amyplanyy_do_emit(struct amyplanyy *amyplanyy)
{
  return 1;
}
static inline int is_autocall(struct amyplanyy *amyplanyy)
{
  return 0;
}
void add_corresponding_get(struct amyplanyy *amyplanyy, double get)
{
  uint16_t uget = (uint16_t)get;
  amyplanyy_add_byte(amyplanyy, uget);
}
void add_corresponding_set(struct amyplanyy *amyplanyy, double get)
{
  uint16_t uset = (uint16_t)get_corresponding_set((uint16_t)get);
  amyplanyy_add_byte(amyplanyy, uset);
}

#define get_abce(stiryy) (&((stiryy)->abce))

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
%token DYNLUACALL LEXLUACALL

%token AT ATTAB NEWLINE TOSTRING TONUMBER

%token EQUALS QMEQUALS PLUSEQUALS COLON COMMA
%token <str> STRING_LITERAL
%token <d> NUMBER
%token <s> VARREF_LITERAL
%token MAYBE_CALL
%token LT GT LE GE
%token FUNCTION ENDFUNCTION LOCVAR

%token BEGINSCOPE BEGINHOLEYSCOPE ENDSCOPE
%token FOR ENDFOR

%token DYNO LEXO IMMO DYN LEX IMM SCOPE
%token IF ELSE ELSEIF ENDIF WHILE ENDWHILE ONCE ENDONCE BREAK CONTINUE
%token D L I DO LO IO DP LP IP DPO LPO IPO LOC
%token APPEND APPEND_LIST
%token RETURN PRINT PERIOD

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
%token DUMMY_TOK1
%token DUMMY_TOK2
%token DUMMY_TOK3

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
%type<d> varref
%type<d> maybe_maybe_call
%type<d> maybeqmequals
%type<d> scopetype
%type<d> beginscope
%type<s> maybe_name


%start st

%%

st: amyplanrules;

/* Start of copypaste */

amyplanrules:
| amyplanrules NEWLINE
| amyplanrules assignrule
| amyplanrules FUNCTION VARREF_LITERAL
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy->ctx = amyplan_locvarctx_alloc(NULL, 2, (size_t)-1, (size_t)-1);
  }
}
OPEN_PAREN maybe_parlist CLOSE_PAREN NEWLINE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    size_t funloc = get_abce(amyplanyy)->bytecodesz;
    amyplanyy_add_fun_sym(amyplanyy, $3, 0, funloc);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_FUN_HEADER);
    amyplanyy_add_double(amyplanyy, amyplanyy->ctx->args);
  }
}
  funlines
  ENDFUNCTION NEWLINE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_NIL); // retval
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, amyplanyy->ctx->args); // argcnt
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, amyplanyy->ctx->sz - amyplanyy->ctx->args); // locvarcnt
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_RETEX2);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_FUN_TRAILER);
    amyplanyy_add_double(amyplanyy, amyplan_symbol_add(amyplanyy, $3, strlen($3)));
    amyplan_locvarctx_free(amyplanyy->ctx);
    amyplanyy->ctx = NULL;
  }
  free($3);
}
| amyplanrules
  beginscope maybe_name NEWLINE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    size_t oldscopeidx = get_abce(amyplanyy)->dynscope.u.area->u.sc.locidx;
    struct abce_mb oldscope;
    struct abce_mb key;
    void *ud = abce_scope_get_userdata(&get_abce(amyplanyy)->dynscope);

    if ($3)
    {
      key = abce_mb_create_string(get_abce(amyplanyy), $3, strlen($3));
    }
    else
    {
      key.typ = ABCE_T_N;
    }
    abce_push_mb(get_abce(amyplanyy), &key); // for GC to see it

    oldscope = get_abce(amyplanyy)->dynscope;
    oldscopeidx = oldscope.u.area->u.sc.locidx;
    get_abce(amyplanyy)->dynscope = abce_mb_create_scope(get_abce(amyplanyy), ABCE_DEFAULT_SCOPE_SIZE, &oldscope, (int)$2);

    if (get_abce(amyplanyy)->dynscope.typ == ABCE_T_N)
    {
      fprintf(stderr, "out of memory\n");
      YYABORT;
    }
    if ($3)
    {
      abce_sc_replace_val_mb(get_abce(amyplanyy), &oldscope, &key, &get_abce(amyplanyy)->dynscope);
    }
    abce_scope_set_userdata(&get_abce(amyplanyy)->dynscope, ud);
    abce_pop(get_abce(amyplanyy));
    if ($3)
    {
      abce_mb_refdn(get_abce(amyplanyy), &key);
    }
    abce_mb_refdn(get_abce(amyplanyy), &oldscope);
    $<d>$ = oldscopeidx;
  }
  free($3);
}
  amyplanrules
  ENDSCOPE NEWLINE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    struct abce_mb creatscope;
    creatscope = get_abce(amyplanyy)->dynscope;
    get_abce(amyplanyy)->dynscope = abce_mb_refup(get_abce(amyplanyy), &get_abce(amyplanyy)->cachebase[(size_t)$<d>5]);
    abce_mb_refdn(get_abce(amyplanyy), &creatscope);
  }
}
| amyplanrules custom_rule
;

beginscope: BEGINSCOPE {$$ = 0;} | BEGINHOLEYSCOPE {$$ = 1;} ;
maybe_name: {$$ = NULL;} | VARREF_LITERAL {$$ = $1; } ;

maybeqmequals: EQUALS {$$ = 0;} | QMEQUALS {$$ = 1;} ;
maybe_maybe_call: {$$ = 0;} | MAYBE_CALL {$$ = 1;};

assignrule:
  VARREF_LITERAL maybe_maybe_call maybeqmequals
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    size_t funloc = get_abce(amyplanyy)->bytecodesz;
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_FUN_HEADER);
    amyplanyy_add_double(amyplanyy, 0);
    $<d>$ = funloc;
  }
}
expr NEWLINE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    unsigned char tmpbuf[256] = {};
    size_t tmpsiz = 0;
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_RET);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_FUN_TRAILER);
    amyplanyy_add_double(amyplanyy, amyplan_symbol_add(amyplanyy, $1, strlen($1)));

    if ($2)
    {
      amyplanyy_add_fun_sym(amyplanyy, $1, $3, $<d>4);
    }
    else if (!$3 || abce_sc_get_rec_str_area(get_abce(amyplanyy)->dynscope.u.area, $1, 1) == NULL)
    {
      if (get_abce(amyplanyy)->sp != 0)
      {
        abort();
      }
      abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_DBL);
      abce_add_double_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), $<d>4);
        //abce_sc_get_rec_str_fun(&get_abce(amyplanyy)->dynscope, $1, 1));
      abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_FUNIFY);
      abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_CALL_IF_FUN);
      abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_EXIT);

      get_abce(amyplanyy)->ip = -tmpsiz-ABCE_GUARD;
      if (abce_engine(get_abce(amyplanyy), tmpbuf, tmpsiz) != 0)
      {
        size_t i;
        printf("Error executing bytecode for var %s\n", $1);
        printf("error %s\n", abce_err_to_str(get_abce(amyplanyy)->err.code));
        printf("Backtrace:\n");
        for (i = 0; i < get_abce(amyplanyy)->btsz; i++)
        {
          if (get_abce(amyplanyy)->btbase[i].typ == ABCE_T_S)
          {
            printf("%s\n", get_abce(amyplanyy)->btbase[i].u.area->u.str.buf);
          }
          else
          {
            printf("(-)\n");
          }
        }
        printf("Additional information:\n");
        abce_mb_dump(&get_abce(amyplanyy)->err.mb);
        amyplanyyerror(scanner, amyplanyy, "error in assignment");
        YYABORT;
      }
      if (get_abce(amyplanyy)->sp != 1)
      {
        abort();
      }
      struct abce_mb key = abce_mb_create_string(get_abce(amyplanyy), $1, strlen($1));
      abce_sc_replace_val_mb(get_abce(amyplanyy), &get_abce(amyplanyy)->dynscope, &key, &get_abce(amyplanyy)->stackbase[0]);
      abce_mb_refdn(get_abce(amyplanyy), &key);
      abce_pop(get_abce(amyplanyy));
    }
  }

  free($1);
}
| VARREF_LITERAL maybe_maybe_call PLUSEQUALS
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    size_t funloc = get_abce(amyplanyy)->bytecodesz;
    size_t oldloc = amyplanyy_add_fun_sym(amyplanyy, $1, 0, funloc); // FIXME move later
    if (oldloc == (size_t)-1)
    {
      printf("Can't find old symbol function for %s\n", $1);
      YYABORT;
    }
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_FUN_HEADER);
    amyplanyy_add_double(amyplanyy, 0);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, oldloc);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CALL_IF_FUN);
    // FIXME what if it's not a list?
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_DUP_NONRECURSIVE);
  }
}
expr NEWLINE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    unsigned char tmpbuf[256] = {};
    size_t tmpsiz = 0;
    size_t symidx;
    // FIXME what if it's not a list?
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_APPENDALL_MAINTAIN);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_RET);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_FUN_TRAILER);
    symidx = amyplan_symbol_add(amyplanyy, $1, strlen($1));
    amyplanyy_add_double(amyplanyy, symidx);

    if (!$2)
    {
      if (get_abce(amyplanyy)->sp != 0)
      {
        abort();
      }
      abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_PUSH_DBL);
      abce_add_double_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf),
        abce_sc_get_rec_str_fun(&get_abce(amyplanyy)->dynscope, $1, 1));
      abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_FUNIFY);
      abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_CALL_IF_FUN);
      abce_add_ins_alt(tmpbuf, &tmpsiz, sizeof(tmpbuf), ABCE_OPCODE_EXIT);

      get_abce(amyplanyy)->ip = -tmpsiz-ABCE_GUARD;
      if (abce_engine(get_abce(amyplanyy), tmpbuf, tmpsiz) != 0)
      {
        size_t i;
        printf("Error executing bytecode for var %s\n", $1);
        printf("error %s\n", abce_err_to_str(get_abce(amyplanyy)->err.code));
        printf("Backtrace:\n");
        for (i = 0; i < get_abce(amyplanyy)->btsz; i++)
        {
          if (get_abce(amyplanyy)->btbase[i].typ == ABCE_T_S)
          {
            printf("%s\n", get_abce(amyplanyy)->btbase[i].u.area->u.str.buf);
          }
          else
          {
            printf("(-)\n");
          }
        }
        printf("Additional information:\n");
        abce_mb_dump(&get_abce(amyplanyy)->err.mb);
        amyplanyyerror(scanner, amyplanyy, "error in += assignment");
        YYABORT;
      }
      if (get_abce(amyplanyy)->sp != 1)
      {
        abort();
      }
      struct abce_mb key = abce_mb_create_string(get_abce(amyplanyy), $1, strlen($1));
      abce_sc_replace_val_mb(get_abce(amyplanyy), &get_abce(amyplanyy)->dynscope, &key, &get_abce(amyplanyy)->stackbase[0]);
      abce_mb_refdn(get_abce(amyplanyy), &key);
      abce_pop(get_abce(amyplanyy));
    }
  }

  free($1);
}
;

maybe_parlist:
| parlist
;

parlist:
VARREF_LITERAL
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplan_locvarctx_add_param(amyplanyy->ctx, $1);
  }
  free($1);
}
| parlist COMMA VARREF_LITERAL
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplan_locvarctx_add_param(amyplanyy->ctx, $3);
  }
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
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplan_locvarctx_add(amyplanyy->ctx, $3);
  }
  free($3);
}
| locvarlines NEWLINE
;

bodylines:
| statement NEWLINE bodylinescont
;

bodylinescont:
| bodylinescont statement NEWLINE
| bodylinescont NEWLINE
;

statement:
  lvalue EQUALS SUB
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    if ($1 != ABCE_OPCODE_DICTSET_MAINTAIN)
    {
      printf("Can remove only from dict\n");
      YYABORT;
    }
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_DICTDEL);
  }
}
| lvalue EQUALS expr
{
  if (amyplanyy_do_emit(amyplanyy))
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
    if ($1 == ABCE_OPCODE_PUSH_FROM_CACHE)
    {
      printf("Can't assign to immediate varref\n");
      YYABORT;
    }
    if (   $1 == ABCE_OPCODE_STRLEN || $1 == ABCE_OPCODE_LISTLEN
        || $1 == ABCE_OPCODE_DICTLEN)
    {
      printf("Can't assign to length query (except for PB)\n");
      YYABORT;
    }
    add_corresponding_set(amyplanyy, $1);
    if ($1 == ABCE_OPCODE_DICTGET) // prev. changes from GET to SET_MAINTAIN
    {
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_POP);
    }
  }
}
| RETURN expr
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, amyplan_locvarctx_arg_sz(amyplanyy->ctx)); // arg
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, amyplan_locvarctx_recursive_sz(amyplanyy->ctx)); // loc
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_RETEX2);
  }
}
| BREAK
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    int64_t loc = amyplan_locvarctx_break(amyplanyy->ctx, 1);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FALSE);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, loc);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_JMP);
  }
}
| CONTINUE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    int64_t loc = amyplan_locvarctx_continue(amyplanyy->ctx, 1);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, loc);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_JMP);
  }
}
| BREAK NUMBER
{
  if (amyplanyy_do_emit(amyplanyy))
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
}
| CONTINUE NUMBER
{
  if (amyplanyy_do_emit(amyplanyy))
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
}
| expr
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_POP); // called for side effects only
  }
}
| IF OPEN_PAREN expr CLOSE_PAREN NEWLINE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    $<d>$ = get_abce(amyplanyy)->bytecodesz;
    amyplanyy_add_double(amyplanyy, -50); // to be overwritten
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_IF_NOT_JMP);
  }
}
  bodylinescont
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_set_double(amyplanyy, $<d>6, get_abce(amyplanyy)->bytecodesz);
    $<d>$ = $<d>6; // For overwrite by maybe_else
  }
}
  maybe_elseifs
  maybe_else
  ENDIF
| FOR OPEN_PAREN statement COMMA
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    $<d>$ = get_abce(amyplanyy)->bytecodesz; // midpoint, $5
  }
}
  expr
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    $<d>$ = get_abce(amyplanyy)->bytecodesz; // addressof_breakpoint, $7
    amyplanyy_add_double(amyplanyy, -50); // to be overwritten
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_JMP);
  }
}
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    $<d>$ = get_abce(amyplanyy)->bytecodesz; // startpoint, $8
  }
}
  COMMA statement CLOSE_PAREN NEWLINE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    struct amyplan_locvarctx *ctx;
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, $<d>5); // addressof_midpoint
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_JMP);
    ctx =
      amyplan_locvarctx_alloc(amyplanyy->ctx, 0, get_abce(amyplanyy)->bytecodesz, $<d>8);
    if (ctx == NULL)
    {
      printf("Out of memory\n");
      YYABORT;
    }
    amyplanyy->ctx = ctx;
    amyplanyy_set_double(amyplanyy, $<d>7, get_abce(amyplanyy)->bytecodesz);
    $<d>$ = get_abce(amyplanyy)->bytecodesz; // breakpoint
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
  }
}
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    $<d>$ = get_abce(amyplanyy)->bytecodesz;
    amyplanyy_add_double(amyplanyy, -50); // addressof_endpoint, $14
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_IF_NOT_JMP);
  }
}
  bodylinescont
  ENDFOR
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    struct amyplan_locvarctx *ctx = amyplanyy->ctx->parent;
    free(amyplanyy->ctx);
    amyplanyy->ctx = ctx;

    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, $<d>8); // addressof_startpoint
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_JMP);
    amyplanyy_set_double(amyplanyy, $<d>14, get_abce(amyplanyy)->bytecodesz);
  }
}
| WHILE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    $<d>$ = get_abce(amyplanyy)->bytecodesz; // startpoint
  }
}
  OPEN_PAREN expr CLOSE_PAREN NEWLINE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    struct amyplan_locvarctx *ctx =
      amyplan_locvarctx_alloc(amyplanyy->ctx, 0, get_abce(amyplanyy)->bytecodesz, $<d>2);
    if (ctx == NULL)
    {
      printf("Out of memory\n");
      YYABORT;
    }
    amyplanyy->ctx = ctx;
    $<d>$ = get_abce(amyplanyy)->bytecodesz; // breakpoint
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, -50); // to be overwritten
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_IF_NOT_JMP);
  }
}
  bodylinescont
  ENDWHILE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    struct amyplan_locvarctx *ctx = amyplanyy->ctx->parent;
    free(amyplanyy->ctx);
    amyplanyy->ctx = ctx;
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, $<d>2);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_JMP);
    amyplanyy_set_double(amyplanyy, $<d>7 + 1, get_abce(amyplanyy)->bytecodesz);
  }
}
| ONCE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    $<d>$ = get_abce(amyplanyy)->bytecodesz; // startpoint
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_TRUE);
  }
}
  NEWLINE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    struct amyplan_locvarctx *ctx =
      amyplan_locvarctx_alloc(amyplanyy->ctx, 0, get_abce(amyplanyy)->bytecodesz, $<d>2);
    if (ctx == NULL)
    {
      printf("Out of memory\n");
      YYABORT;
    }
    amyplanyy->ctx = ctx;
    $<d>$ = get_abce(amyplanyy)->bytecodesz; // breakpoint
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, -50); // to be overwritten
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_IF_NOT_JMP);
  }
}
  bodylinescont
  ENDONCE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    struct amyplan_locvarctx *ctx = amyplanyy->ctx->parent;
    free(amyplanyy->ctx);
    amyplanyy->ctx = ctx;
    amyplanyy_set_double(amyplanyy, $<d>4 + 1, get_abce(amyplanyy)->bytecodesz);
  }
}
| APPEND OPEN_PAREN expr COMMA expr CLOSE_PAREN
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_APPEND_MAINTAIN);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_POP);
  }
}
| APPEND_LIST OPEN_PAREN expr COMMA expr CLOSE_PAREN
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_APPENDALL_MAINTAIN);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_POP);
  }
}
| STDOUT OPEN_PAREN expr CLOSE_PAREN
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, 0);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_OUT);
  }
}
| STDERR OPEN_PAREN expr CLOSE_PAREN
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, 1);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_OUT);
  }
}
| ERROR OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_ERROR); }
| DUMP OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_DUMP); }
| EXIT OPEN_PAREN CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_EXIT); }
| custom_stmt
;

maybe_elseifs:
{
  $<d>$ = $<d>0;
}
| maybe_elseifs ELSEIF
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    $<d>$ = get_abce(amyplanyy)->bytecodesz;
    amyplanyy_add_double(amyplanyy, -50); // to be overwritten
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_JMP);
    amyplanyy_set_double(amyplanyy, $<d>1, get_abce(amyplanyy)->bytecodesz); // Overwrite, mid1
  }
}
  OPEN_PAREN expr CLOSE_PAREN NEWLINE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    $<d>$ = get_abce(amyplanyy)->bytecodesz;
    amyplanyy_add_double(amyplanyy, -50); // to be overwritten, FIXME!
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_IF_NOT_JMP);
  }
}
  bodylinescont
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_set_double(amyplanyy, $<d>8, get_abce(amyplanyy)->bytecodesz);
    amyplanyy_set_double(amyplanyy, $<d>3, get_abce(amyplanyy)->bytecodesz);
  }
  $<d>$ = $<d>8; // for overwrite by maybe_else
}
;

maybe_else:
| ELSE NEWLINE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    $<d>$ = get_abce(amyplanyy)->bytecodesz;
    amyplanyy_add_double(amyplanyy, -50); // to be overwritten
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_JMP);
    amyplanyy_set_double(amyplanyy, $<d>0, get_abce(amyplanyy)->bytecodesz); // Overwrite
  }
}
bodylinescont
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_set_double(amyplanyy, $<d>3, get_abce(amyplanyy)->bytecodesz);
  }
}
;

varref_tail:
  OPEN_BRACKET expr CLOSE_BRACKET
{
  $$ = ABCE_OPCODE_LISTGET;
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
  $$ = ABCE_OPCODE_DICTGET;
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
  $$ = ABCE_OPCODE_PBGET; // FIXME needs transfer size of operation
}
| OPEN_BRACE AT CLOSE_BRACE
{
  $$ = ABCE_OPCODE_PBLEN; // This is very special: CAN assign to length query
}
;

lvalue:
  varref
{
  $$ = $1;
}
| varref
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, $1);
  }
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
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, $1);
  }
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
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, $1);
  }
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
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, $1);
  }
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
  $$ = ABCE_OPCODE_SCOPEVAR;
}
| ATQM
{
  $$ = ABCE_OPCODE_SCOPE_HAS;
}
;

lexstart:
  LEX
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, get_abce(amyplanyy)->dynscope.u.area->u.sc.locidx);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
  }
}
  OPEN_BRACKET maybe_atqm expr CLOSE_BRACKET
{
  $$ = $4;
}
;

dynstart:
  DYN
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_GETSCOPE_DYN);
  }
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
  if (amyplanyy_do_emit(amyplanyy))
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
  if (amyplanyy_do_emit(amyplanyy))
  {
    int64_t locvar;
    if (amyplanyy->ctx == NULL)
    {
      // Outside of function, search for dynamic symbol
      // Doesn't really happen with standard syntax, but somebody may embed it
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_GETSCOPE_DYN);

      int64_t idx = abce_cache_add_str(get_abce(amyplanyy), $1, strlen($1));
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
      amyplanyy_add_double(amyplanyy, idx);
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
      $$ = ABCE_OPCODE_SCOPEVAR;
    }
    else
    {
      locvar = amyplan_locvarctx_search_rec(amyplanyy->ctx, $1);
      if (locvar >= 0)
      {
        amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
        amyplanyy_add_double(amyplanyy, locvar);
      }
      else
      {
        printf("var %s not found\n", $1);
        YYABORT;
      }
      $$ = ABCE_OPCODE_PUSH_STACK;
    }
  }
  free($1);
}
| IO VARREF_LITERAL
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    const struct abce_mb *mb2 =
      abce_sc_get_rec_str(&get_abce(amyplanyy)->dynscope, $2, 0);
    if (mb2 == NULL)
    {
      printf("Variable %s not found\n", $2);
      YYABORT;
    }
    int64_t idx = abce_cache_add(get_abce(amyplanyy), mb2);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, idx);
  }
  free($2);
  $$ = ABCE_OPCODE_PUSH_FROM_CACHE;
}
| I VARREF_LITERAL
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    const struct abce_mb *mb2 =
      abce_sc_get_rec_str(&get_abce(amyplanyy)->dynscope, $2, 1);
    if (mb2 == NULL)
    {
      printf("Variable %s not found\n", $2);
      YYABORT;
    }
    int64_t idx = abce_cache_add(get_abce(amyplanyy), mb2);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, idx);
  }
  free($2);
  $$ = ABCE_OPCODE_PUSH_FROM_CACHE;
}
| IP VARREF_LITERAL
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    if (get_abce(amyplanyy)->dynscope.u.area->u.sc.parent == NULL)
    {
      printf("No parent scope, can't use immediate parent reference\n");
      YYABORT;
    }
    struct abce_mb mb1 = {.typ = ABCE_T_SC, .u = {.area = get_abce(amyplanyy)->dynscope.u.area->u.sc.parent}};
    const struct abce_mb *mb2 = abce_sc_get_rec_str(&mb1, $2, 1);
    if (mb2 == NULL)
    {
      printf("Variable %s not found\n", $2);
      YYABORT;
    }
    int64_t idx = abce_cache_add(get_abce(amyplanyy), mb2);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, idx);
  }
  free($2);
  $$ = ABCE_OPCODE_PUSH_FROM_CACHE;
}
| IPO VARREF_LITERAL
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    if (get_abce(amyplanyy)->dynscope.u.area->u.sc.parent == NULL)
    {
      printf("No parent scope, can't use immediate parent reference\n");
      YYABORT;
    }
    struct abce_mb mb1 = {.typ = ABCE_T_SC, .u = {.area = get_abce(amyplanyy)->dynscope.u.area->u.sc.parent}};
    const struct abce_mb *mb2 = abce_sc_get_rec_str(&mb1, $2, 0);
    if (mb2 == NULL)
    {
      printf("Variable %s not found\n", $2);
      YYABORT;
    }
    int64_t idx = abce_cache_add(get_abce(amyplanyy), mb2);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, idx);
  }
  free($2);
  $$ = ABCE_OPCODE_PUSH_FROM_CACHE;
}
| scopetype VARREF_LITERAL
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    int64_t idx = abce_cache_add_str(get_abce(amyplanyy), $2, strlen($2));
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, idx);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
  }
  free($2);
  if ($1)
  {
    $$ = ABCE_OPCODE_SCOPEVAR_NONRECURSIVE;
  }
  else
  {
    $$ = ABCE_OPCODE_SCOPEVAR;
  }
}
;

scopetype:
  LP
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    if (get_abce(amyplanyy)->dynscope.u.area->u.sc.parent == NULL)
    {
      printf("No parent scope, can't use lexical parent reference\n");
      YYABORT;
    }
    amyplanyy_add_double(amyplanyy, get_abce(amyplanyy)->dynscope.u.area->u.sc.parent->u.sc.locidx);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
  }
  $$ = 0;
}
| LPO
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    if (get_abce(amyplanyy)->dynscope.u.area->u.sc.parent == NULL)
    {
      printf("No parent scope, can't use lexical parent reference\n");
      YYABORT;
    }
    amyplanyy_add_double(amyplanyy, get_abce(amyplanyy)->dynscope.u.area->u.sc.parent->u.sc.locidx);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
  }
  $$ = 1;
}
| L
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, get_abce(amyplanyy)->dynscope.u.area->u.sc.locidx);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
  }
  $$ = 0;
}
| LO
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, get_abce(amyplanyy)->dynscope.u.area->u.sc.locidx);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
  }
  $$ = 1;
}
| DP
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_GETSCOPE_DYN);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_SCOPE_PARENT);
  }
  $$ = 0;
}
| D
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_GETSCOPE_DYN);
  }
  $$ = 0;
}
| DO
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_GETSCOPE_DYN);
  }
  $$ = 1;
}
| DPO
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_GETSCOPE_DYN);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_SCOPE_PARENT);
  }
  $$ = 1;
}
;

expr: expr11;

expr1:
  expr0_or_string
| LOGICAL_NOT expr1
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LOGICAL_NOT);
  }
}
| BITWISE_NOT expr1
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_BITWISE_NOT);
  }
}
| ADD expr1
| SUB expr1
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_UNARY_MINUS);
  }
}
;

expr2:
  expr1
| expr2 MUL expr1
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_MUL);
  }
}
| expr2 DIV expr1
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_DIV);
  }
}
| expr2 MOD expr1
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_MOD);
  }
}
;

expr3:
  expr2
| expr3 ADD expr2
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_ADD);
  }
}
| expr3 SUB expr2
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_SUB);
  }
}
| expr3 PERIOD expr2
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRAPPEND);
  }
}
;

expr4:
  expr3
| expr4 SHL expr3
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_SHL);
  }
}
| expr4 SHR expr3
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_SHR);
  }
}
;

expr5:
  expr4
| expr5 LT expr4
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LT);
  }
}
| expr5 LE expr4
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LE);
  }
}
| expr5 GT expr4
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_GT);
  }
}
| expr5 GE expr4
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_GE);
  }
}
;

expr6:
  expr5
| expr6 EQ expr5
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_EQ);
  }
}
| expr6 NE expr5
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_NE);
  }
}
;

expr7:
  expr6
| expr7 BITWISE_AND expr6
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_BITWISE_AND);
  }
}
;

expr8:
  expr7
| expr8 BITWISE_XOR expr7
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_BITWISE_XOR);
  }
}
;

expr9:
  expr8
| expr9 BITWISE_OR expr8
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_BITWISE_OR);
  }
}
;

expr10:
  expr9
| expr10 LOGICAL_AND
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_TOP);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    $<d>$ = get_abce(amyplanyy)->bytecodesz;
    amyplanyy_add_double(amyplanyy, 0);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_IF_NOT_JMP);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_POP);
  }
}
  expr9
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_set_double(amyplanyy, $<d>3, get_abce(amyplanyy)->bytecodesz);
  }
}
;

expr11:
  expr10
| expr11 LOGICAL_OR
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_TOP);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LOGICAL_NOT);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    $<d>$ = get_abce(amyplanyy)->bytecodesz;
    amyplanyy_add_double(amyplanyy, 0);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_IF_NOT_JMP);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_POP);
  }
}
  expr10
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_set_double(amyplanyy, $<d>3, get_abce(amyplanyy)->bytecodesz);
  }
}
;

expr0_or_string:
  expr0_without_string
| STRING_LITERAL
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    int64_t idx = abce_cache_add_str(get_abce(amyplanyy), $1.str, $1.sz);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, idx);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
  }
  free($1.str);
}
;

expr0_without_string:
  OPEN_PAREN expr CLOSE_PAREN
| OPEN_PAREN expr CLOSE_PAREN OPEN_PAREN maybe_arglist CLOSE_PAREN
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, $5);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CALL);
  }
}
| OPEN_PAREN expr CLOSE_PAREN MAYBE_CALL
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CALL_IF_FUN);
  }
}
| dict maybe_bracketexprlist
| list maybe_bracketexprlist
| NUMBER
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, $1);
  }
}
| TRUE { if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_TRUE); }
| TYPE OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_TYPE); }
| FALSE { if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FALSE); }
| NIL { if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_NIL); }
| STR_FROMCHR OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STR_FROMCHR); }
| STR_LOWER OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STR_LOWER); }
| STR_UPPER OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STR_UPPER); }
| STR_REVERSE OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STR_REVERSE); }
| STRCMP OPEN_PAREN expr COMMA expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STR_CMP); }
| STRSTR OPEN_PAREN expr COMMA expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRSTR); }
| STRREP OPEN_PAREN expr COMMA expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRREP); }
| STRLISTJOIN OPEN_PAREN expr COMMA expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRLISTJOIN); }
| STRAPPEND OPEN_PAREN expr COMMA expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRAPPEND); }
| STRSTRIP OPEN_PAREN expr COMMA expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRSTRIP); }
| STRSUB OPEN_PAREN expr COMMA expr COMMA expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRSUB); }
| STRGSUB OPEN_PAREN expr COMMA expr COMMA expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRGSUB); }
| STRSET OPEN_PAREN expr COMMA expr COMMA expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRSET); }
| STRWORD OPEN_PAREN expr COMMA expr COMMA expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRWORD); }
| STRWORDCNT OPEN_PAREN expr COMMA expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRWORDCNT); }
| STRWORDLIST OPEN_PAREN expr COMMA expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_STRWORDCNT); }
| ABS OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_ABS); }
| ACOS OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_ACOS); }
| ASIN OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_ASIN); }
| ATAN OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_ATAN); }
| CEIL OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CEIL); }
| COS OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_COS); }
| SIN OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_SIN); }
| TAN OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_TAN); }
| EXP OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_EXP); }
| LOG OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LOG); }
| SQRT OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_SQRT); }
| DUP_NONRECURSIVE OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_DUP_NONRECURSIVE); }
| PB_NEW OPEN_PAREN CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_NEW_PB); }
| TOSTRING OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_TOSTRING); }
| TONUMBER OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_TONUMBER); }
| SCOPE_PARENT OPEN_PAREN expr CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_SCOPE_PARENT); }
| SCOPE_NEW OPEN_PAREN expr COMMA expr CLOSE_PAREN
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_SCOPE_NEW);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
  }
}
| GETSCOPE_DYN OPEN_PAREN CLOSE_PAREN
{ if (amyplanyy_do_emit(amyplanyy)) amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_GETSCOPE_DYN); }
| GETSCOPE_LEX OPEN_PAREN CLOSE_PAREN
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, get_abce(amyplanyy)->dynscope.u.area->u.sc.locidx);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
  }
}
| lvalue
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    add_corresponding_get(amyplanyy, $1);
  }
}
| lvalue
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    add_corresponding_get(amyplanyy, $1);
  }
}
  OPEN_PAREN maybe_arglist CLOSE_PAREN
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, $4);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CALL);
  }
}
| DYNLUACALL
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_GETSCOPE_DYN);
  }
}
  OPEN_PAREN expr COMMA maybe_arglist CLOSE_PAREN
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, $6);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LUACALL);
  }
}
| LEXLUACALL
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, get_abce(amyplanyy)->dynscope.u.area->u.sc.locidx);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_FROM_CACHE);
  }
}
  OPEN_PAREN expr COMMA maybe_arglist CLOSE_PAREN
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_DBL);
    amyplanyy_add_double(amyplanyy, $6);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_LUACALL);
  }
}
| lvalue MAYBE_CALL
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    add_corresponding_get(amyplanyy, $1);
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CALL_IF_FUN);
  }
}
| IMM OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist
{
  fprintf(stderr, "unsupported syntax\n");
  YYABORT;
}
| IMM OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist OPEN_PAREN maybe_arglist CLOSE_PAREN
{
  fprintf(stderr, "unsupported syntax\n");
  YYABORT;
}
| IMM OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist MAYBE_CALL
{
  fprintf(stderr, "unsupported syntax\n");
  YYABORT;
}
| DYNO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist
{
  fprintf(stderr, "unsupported syntax\n");
  YYABORT;
}
| DYNO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist OPEN_PAREN maybe_arglist CLOSE_PAREN
{
  fprintf(stderr, "unsupported syntax\n");
  YYABORT;
}
| DYNO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist MAYBE_CALL
{
  fprintf(stderr, "unsupported syntax\n");
  YYABORT;
}
| LEXO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist
{
  fprintf(stderr, "unsupported syntax\n");
  YYABORT;
}
| LEXO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist OPEN_PAREN maybe_arglist CLOSE_PAREN
{
  fprintf(stderr, "unsupported syntax\n");
  YYABORT;
}
| LEXO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist MAYBE_CALL
{
  fprintf(stderr, "unsupported syntax\n");
  YYABORT;
}
| IMMO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist
{
  fprintf(stderr, "unsupported syntax\n");
  YYABORT;
}
| IMMO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist OPEN_PAREN maybe_arglist CLOSE_PAREN
{
  fprintf(stderr, "unsupported syntax\n");
  YYABORT;
}
| IMMO OPEN_BRACKET expr CLOSE_BRACKET maybe_bracketexprlist MAYBE_CALL
{
  fprintf(stderr, "unsupported syntax\n");
  YYABORT;
}
| LOC OPEN_BRACKET STRING_LITERAL CLOSE_BRACKET maybe_bracketexprlist
{
  fprintf(stderr, "unsupported syntax\n");
  free($3.str);
  YYABORT;
}
| LOC OPEN_BRACKET STRING_LITERAL CLOSE_BRACKET maybe_bracketexprlist OPEN_PAREN maybe_arglist CLOSE_PAREN
{
  fprintf(stderr, "unsupported syntax\n");
  free($3.str);
  YYABORT;
}
| LOC OPEN_BRACKET STRING_LITERAL CLOSE_BRACKET maybe_bracketexprlist MAYBE_CALL
{
  fprintf(stderr, "unsupported syntax\n");
  free($3.str);
  YYABORT;
}
| custom_expr0
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
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_NEW_ARRAY);
  }
}
maybe_valuelist CLOSE_BRACKET
;

dict:
OPEN_BRACE
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_PUSH_NEW_DICT);
  }
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
  value COLON
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    if (is_autocall(amyplanyy))
    {
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CALL_IF_FUN);
    }
  }
}
  value
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    if (is_autocall(amyplanyy))
    {
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CALL_IF_FUN);
    }
    amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_DICTSET_MAINTAIN);
  }
}
;

maybe_valuelist:
| valuelist
;

valuelist:
  valuelistentry
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    if (is_autocall(amyplanyy))
    {
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CALL_IF_FUN);
    }
    if ($1)
    {
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_APPENDALL_MAINTAIN);
    }
    else
    {
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_APPEND_MAINTAIN);
    }
  }
}
| valuelist COMMA valuelistentry
{
  if (amyplanyy_do_emit(amyplanyy))
  {
    if (is_autocall(amyplanyy))
    {
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_CALL_IF_FUN);
    }
    if ($3)
    {
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_APPENDALL_MAINTAIN);
    }
    else
    {
      amyplanyy_add_byte(amyplanyy, ABCE_OPCODE_APPEND_MAINTAIN);
    }
  }
}
;

valuelistentry:
  value
{
  $$ = $1;
};

/* End of copypaste */

custom_stmt: DUMMY_TOK1;
custom_expr0: DUMMY_TOK2;
custom_rule: DUMMY_TOK3;
