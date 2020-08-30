// main.c

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {

   if (argc != 2) {
      fprintf(stderr, "%s: 引数の数が無効です\n", argv[0]);
      return 1;
   }

   char *p = argv[1];

   printf(".globl main\n");
   printf("main:\n");
   printf("  mov $%ld, %%rax\n", strtol(p, &p, 10));

loop:

   while (*p) {

      if (*p == '+') {
         p++;
         goto add;
      }

      if (*p == '-') {
         p++;
         goto sub;
      }

      fprintf(stderr, "予期しない文字: '%c'\n", *p);
      return 1;

add:
   printf("  add $%ld, %%rax\n", strtol(p, &p, 10));
   goto loop;

sub:
   printf("  sub $%ld, %%rax\n", strtol(p, &p, 10));
   goto loop;

   }

   printf("  ret\n");
   return 0;
}

