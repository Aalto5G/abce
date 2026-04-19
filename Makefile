.SUFFIXES:

SRC_LIB := amyplanyyutils.c memblock.c abcerbtree.c amyplanlocvarctx.c engine.c abcestring.c abcetrees.c abcescopes.c abce.c abceapi.c safemode.c abcejmalloc.c abceprettyftoa.c abce_caj_out.c abcestreamingatof.c abce_caj.c
SRC := $(SRC_LIB) amyplantest.c main.c locvartest.c treetest.c fiboefftest.c fibonaccitest.c bttest.c ret.c breaktest.c gctest.c reftest.c fortest.c iftest.c dumptest.c shortcut.c dictnext.c amyplan.c

BYACC ?= byacc
FLEX ?= flex

SRC_CPP_LIB :=
SRC_CPP := $(SRC_CPP_LIB)

LEX_LIB := amyplanyy.l
LEX := $(LEX_LIB)

YACC_LIB := amyplanyy.y
YACC := $(YACC_LIB)

LEXGEN_LIB := $(patsubst %.l,%.lex.c,$(LEX_LIB))
LEXGEN := $(patsubst %.l,%.lex.c,$(LEX))

YACCGEN_LIB := $(patsubst %.y,%.tab.c,$(YACC_LIB))
YACCGEN := $(patsubst %.y,%.tab.c,$(YACC))

GEN_LIB := $(patsubst %.l,%.lex.c,$(LEX_LIB)) $(patsubst %.y,%.tab.c,$(YACC_LIB))
GEN := $(patsubst %.l,%.lex.c,$(LEX)) $(patsubst %.y,%.tab.c,$(YACC))

OBJ_LIB := $(patsubst %.c,%.o,$(SRC_LIB))
OBJ := $(patsubst %.c,%.o,$(SRC))

OBJ_CPP_LIB := $(patsubst %.cc,%.o,$(SRC_CPP_LIB))
OBJ_CPP := $(patsubst %.cc,%.o,$(SRC_CPP))

OBJGEN_LIB := $(patsubst %.c,%.o,$(GEN_LIB))
OBJGEN := $(patsubst %.c,%.o,$(GEN))

ASM_LIB := $(patsubst %.c,%.s,$(SRC_LIB))
ASM := $(patsubst %.c,%.s,$(SRC))

ASMGEN_LIB := $(patsubst %.c,%.s,$(GEN_LIB))
ASMGEN := $(patsubst %.c,%.s,$(GEN))

DEP_LIB := $(patsubst %.c,%.d,$(SRC_LIB))
DEP := $(patsubst %.c,%.d,$(SRC))

DEP_CPP_LIB := $(patsubst %.cc,%.d,$(SRC_CPP_LIB))
DEP_CPP := $(patsubst %.cc,%.d,$(SRC_CPP))

DEPGEN_LIB := $(patsubst %.c,%.d,$(GEN_LIB))
DEPGEN := $(patsubst %.c,%.d,$(GEN))

.PHONY: all wc wclib wcparser

all: amyplantest main locvartest treetest fiboefftest fibonaccitest bttest ret breaktest gctest reftest fortest iftest dumptest shortcut dictnext amyplan

wc:
	wc -l $(LEX) $(YACC) $(SRC_CPP) $(SRC) $(filter-out %.lex.h %.tab.h,$(wildcard *.h))

wclib:
	wc -l $(LEX) $(YACC) $(SRC_CPP) $(SRC_LIB) $(filter-out %.lex.h %.tab.h,$(wildcard *.h))

wcparser:
	wc -l $(LEX) $(YACC) amyplanyy.h amyplanyyutils.[ch] amyplanlocvarctx.[ch]

#LUAINC:=/usr/include/lua5.3
#LUALIB:=/usr/lib/x86_64-linux-gnu/liblua5.3.a
LUAINCS?=
LUALIBS?=

CC?=cc
CPP?=c++
CFLAGS?=-O3 -Wall -Wextra -Wno-unused-parameter -g
CPPFLAGS?=-O3 -Wall -Wextra -Wno-unused-parameter -g

ifeq ($(WITH_LUA),yes)
  CFLAGS += $(LUAINCS) -DWITH_LUA
  CPPFLAGS += $(LUAINCS) -DWITH_LUA
else
  LUALIBS :=
endif

gctest: gctest.o libabce.a Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LUALIBS) -lm

reftest: reftest.o libabce.a Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LUALIBS) -lm

ret: ret.o libabce.a Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LUALIBS) -lm

breaktest: breaktest.o libabce.a Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LUALIBS) -lm

main: main.o libabce.a Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LUALIBS) -lm

locvartest: locvartest.o libabce.a Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LUALIBS) -lm

treetest: treetest.o libabce.a Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LUALIBS) -lm

amyplantest: amyplantest.o libabce.a Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LUALIBS) -lm

fiboefftest: fiboefftest.o libabce.a Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LUALIBS) -lm

fortest: fortest.o libabce.a Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LUALIBS) -lm

amyplan: amyplan.o libabce.a Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LUALIBS) -lm

iftest: iftest.o libabce.a Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LUALIBS) -lm

dumptest: dumptest.o libabce.a Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LUALIBS) -lm

shortcut: shortcut.o libabce.a Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LUALIBS) -lm

dictnext: dictnext.o libabce.a Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LUALIBS) -lm

fibonaccitest: fibonaccitest.o libabce.a Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LUALIBS) -lm

bttest: bttest.o libabce.a Makefile
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) $(LUALIBS) -lm

libabce.a: $(OBJ_LIB) $(OBJGEN_LIB) $(OBJ_CPP_LIB) Makefile
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(OBJ): %.o: %.c %.d Makefile
	$(CC) $(CFLAGS) -c -o $*.o $*.c
$(OBJ_CPP): %.o: %.cc %.d Makefile
	$(CPP) $(CPPFLAGS) -c -o $*.o $*.cc
$(OBJGEN): %.o: %.c %.h %.d Makefile
	$(CC) $(CFLAGS) -c -o $*.o $*.c -Wno-sign-compare -Wno-missing-prototypes

$(DEP): %.d: %.c Makefile
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c
$(DEP_CPP): %.d: %.cc Makefile
	$(CPP) $(CPPFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.cc
$(DEPGEN): %.d: %.c %.h Makefile
	$(CC) $(CFLAGS) -MM -MP -MT "$*.d $*.o" -o $*.d $*.c

amyplanyy.lex.d: amyplanyy.tab.h amyplanyy.lex.h
amyplanyy.lex.o: amyplanyy.tab.h amyplanyy.lex.h
amyplanyy.tab.d: amyplanyy.tab.h amyplanyy.lex.h
amyplanyy.tab.o: amyplanyy.tab.h amyplanyy.lex.h

amyplanyy.lex.c: amyplanyy.l Makefile
	$(FLEX) --outfile=amyplanyy.lex.c --header-file=/dev/null amyplanyy.l
amyplanyy.lex.h: amyplanyy.l Makefile
	$(FLEX) --outfile=/dev/null --header-file=amyplanyy.lex.h amyplanyy.l
amyplanyy.tab.c: amyplanyy.y Makefile
	$(BYACC) -d -p amyplanyy -o .tmpc.amyplanyy.tab.c amyplanyy.y
	rm .tmpc.amyplanyy.tab.h
	mv .tmpc.amyplanyy.tab.c amyplanyy.tab.c
amyplanyy.tab.h: amyplanyy.y Makefile
	$(BYACC) -d -p amyplanyy -o .tmph.amyplanyy.tab.c amyplanyy.y
	rm .tmph.amyplanyy.tab.c
	mv .tmph.amyplanyy.tab.h amyplanyy.tab.h

.PHONY: clean distclean

clean:
	rm -f $(OBJ) $(DEP) $(ASM) $(ASMGEN) $(DEPGEN) $(OBJGEN)
distclean: clean
	rm -f amyplantest main locvartest treetest

-include *.d
