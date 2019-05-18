.SUFFIXES:

SRC_LIB := yyutils.c memblock.c rbtree.c locvarctx.c engine.c string.c trees.c scopes.c abce.c
SRC := $(SRC_LIB) amyplantest.c main.c locvartest.c treetest.c fiboefftest.c

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

.PHONY: all wc

all: amyplantest main locvartest treetest fiboefftest

wc:
	wc -l $(LEX) $(YACC) $(SRC_CPP) $(SRC) $(filter-out %.lex.h %.tab.h,$(wildcard *.h))

#LUAINC:=/usr/include/lua5.3
#LUALIB:=/usr/lib/x86_64-linux-gnu/liblua5.3.a
LUAINC:=/usr/include/luajit-2.1
LUALIB:=/usr/lib/x86_64-linux-gnu/libluajit-5.1.a

CC=clang
CPP=clang++
CFLAGS=-O3 -Wall -g #-I$(LUAINC)
CPPFLAGS=-O3 -Wall -g #-I$(LUAINC)

main: main.o libabce.a Makefile $(LUALIB)
	$(CC) $(CPPFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) -lm -ldl

locvartest: locvartest.o libabce.a Makefile $(LUALIB)
	$(CC) $(CPPFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) -lm -ldl

treetest: treetest.o libabce.a Makefile $(LUALIB)
	$(CC) $(CPPFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) -lm -ldl

amyplantest: amyplantest.o libabce.a Makefile $(LUALIB)
	$(CC) $(CPPFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) -lm -ldl

fiboefftest: fiboefftest.o libabce.a Makefile $(LUALIB)
	$(CC) $(CPPFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) -lm -ldl

libabce.a: $(OBJ_LIB) $(OBJGEN_LIB) $(OBJ_CPP_LIB) Makefile
	rm -f $@
	ar rvs $@ $(filter %.o,$^)

$(OBJ): %.o: %.c %.d Makefile
	$(CC) $(CFLAGS) -c -o $*.o $*.c
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c
$(OBJ_CPP): %.o: %.cc %.d Makefile
	$(CPP) $(CPPFLAGS) -c -o $*.o $*.cc
	$(CPP) $(CPPFLAGS) -c -S -o $*.s $*.cc
$(OBJGEN): %.o: %.c %.h %.d Makefile
	$(CC) $(CFLAGS) -c -o $*.o $*.c -Wno-sign-compare -Wno-missing-prototypes
	$(CC) $(CFLAGS) -c -S -o $*.s $*.c -Wno-sign-compare -Wno-missing-prototypes

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
	flex --outfile=amyplanyy.lex.c --header-file=/dev/null amyplanyy.l
amyplanyy.lex.h: amyplanyy.l Makefile
	flex --outfile=/dev/null --header-file=amyplanyy.lex.h amyplanyy.l
amyplanyy.tab.c: amyplanyy.y Makefile
	byacc -d -p amyplanyy -o .tmpc.amyplanyy.tab.c amyplanyy.y
	rm .tmpc.amyplanyy.tab.h
	mv .tmpc.amyplanyy.tab.c amyplanyy.tab.c
amyplanyy.tab.h: amyplanyy.y Makefile
	byacc -d -p amyplanyy -o .tmph.amyplanyy.tab.c amyplanyy.y
	rm .tmph.amyplanyy.tab.c
	mv .tmph.amyplanyy.tab.h amyplanyy.tab.h

.PHONY: clean distclean

clean:
	rm -f $(OBJ) $(DEP) $(ASM) $(ASMGEN) $(DEPGEN) $(OBJGEN)
distclean: clean
	rm -f amyplantest main locvartest treetest

-include *.d
