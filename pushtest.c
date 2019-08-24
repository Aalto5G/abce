#include "abce.h"

/*

  Invoke this file with the following change:

diff --git a/engine.c b/engine.c
index 1f3242f..7e8aa33 100644
--- a/engine.c
+++ b/engine.c
@@ -1442,7 +1442,20 @@ int abce_engine(struct abce *abce, unsigned char *addcode, size_t addsz)
       switch (ins)
       {
         case ABCE_OPCODE_NOP:
+        {
+          uint8_t b;
+          if (abce_fetch_b(&b, abce, addcode, addsz) != 0)
+          {
+            ret = -EFAULT;
+            break;
+          }
+          if (abce_push_double(abce, (double)(char)b) != 0)
+          {
+            ret = -EFAULT;
+            break;
+          }
           break;
+        }
         case ABCE_OPCODE_PUSH_DBL:
         {
           double dbl;

  ..to prove that a byte-push instruction is not worth it, only giving
  approximately 15% speedup.

 */

int main(int argc, char **argv)
{
  struct abce abce = {};

  abce_init(&abce);

  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 1000000);
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_TRUE); // stack: iter bool

#if 1
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 5);

  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 10);
  abce_add_ins(&abce, ABCE_OPCODE_POP_MANY);
#else
  abce_add_ins(&abce, ABCE_OPCODE_NOP);
  abce_add_ins(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_NOP);
  abce_add_ins(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_NOP);
  abce_add_ins(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_NOP);
  abce_add_ins(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_NOP);
  abce_add_ins(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_NOP);
  abce_add_ins(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_NOP);
  abce_add_ins(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_NOP);
  abce_add_ins(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_NOP);
  abce_add_ins(&abce, 5);
  abce_add_ins(&abce, ABCE_OPCODE_NOP);
  abce_add_ins(&abce, 5);

  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 10);
  abce_add_ins(&abce, ABCE_OPCODE_POP_MANY);
#endif

#if 0
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 0);
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_STACK); // old val
  abce_add_ins(&abce, ABCE_OPCODE_DUMP); // old val
#endif

  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 0); // set stack arg 0 // stack: iter bool 0
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 0); // stack: iter bool 0 0
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_STACK); // stack: iter bool 0 iter
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 1); // stack: iter bool 0 iter 1
  abce_add_ins(&abce, ABCE_OPCODE_SUB); // stack: iter bool 0 iter-1
  abce_add_ins(&abce, ABCE_OPCODE_SET_STACK); // stack: iter bool

  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 0);
  abce_add_ins(&abce, ABCE_OPCODE_PUSH_STACK); // stack: iter bool iter

  abce_add_ins(&abce, ABCE_OPCODE_LOGICAL_NOT);

  abce_add_ins(&abce, ABCE_OPCODE_PUSH_DBL);
  abce_add_double(&abce, 10); // jmp addr, stack: iter bool iter 10

  abce_add_ins(&abce, ABCE_OPCODE_IF_NOT_JMP); // stack: iter bool

  printf("ret %d\n", abce_engine(&abce, NULL, 0));
  return 0;
}
