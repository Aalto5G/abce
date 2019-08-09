.SUFFIXES:

SRC_LIB := amyplanyyutils.c memblock.c abcerbtree.c amyplanlocvarctx.c engine.c abcestring.c abcetrees.c abcescopes.c abce.c abceapi.c safemode.c
SRC := $(SRC_LIB) amyplantest.c main.c locvartest.c treetest.c fiboefftest.c fibonaccitest.c bttest.c ret.c breaktest.c gctest.c reftest.c

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

all: amyplantest main locvartest treetest fiboefftest fibonaccitest bttest ret breaktest gctest reftest

wc:
	wc -l $(LEX) $(YACC) $(SRC_CPP) $(SRC) $(filter-out %.lex.h %.tab.h,$(wildcard *.h))

wclib:
	wc -l $(LEX) $(YACC) $(SRC_CPP) $(SRC_LIB) $(filter-out %.lex.h %.tab.h,$(wildcard *.h))

wcparser:
	wc -l $(LEX) $(YACC) amyplanyy.h amyplanyyutils.[ch] amyplanlocvarctx.[ch]

#LUAINC:=/usr/include/lua5.3
#LUALIB:=/usr/lib/x86_64-linux-gnu/liblua5.3.a
LUAINC:=/usr/include/luajit-2.1
LUALIB:=/usr/lib/x86_64-linux-gnu/libluajit-5.1.a
LUALIB:=

CC=clang
CPP=clang++
CFLAGS=-O3 -Wall -g #-I$(LUAINC)
CPPFLAGS=-O3 -Wall -g #-I$(LUAINC)

gctest: gctest.o libabce.a Makefile $(LUALIB)
	$(CC) $(CPPFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) -lm -ldl

reftest: reftest.o libabce.a Makefile $(LUALIB)
	$(CC) $(CPPFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) -lm -ldl

ret: ret.o libabce.a Makefile $(LUALIB)
	$(CC) $(CPPFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) -lm -ldl

breaktest: breaktest.o libabce.a Makefile $(LUALIB)
	$(CC) $(CPPFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) -lm -ldl

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

fibonaccitest: fibonaccitest.o libabce.a Makefile $(LUALIB)
	$(CC) $(CPPFLAGS) -o $@ $(filter %.o,$^) $(filter %.a,$^) -lm -ldl

bttest: bttest.o libabce.a Makefile $(LUALIB)
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
