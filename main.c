// main.c

#include "1cc.h"

int main(int argc, char **argv) {

   if (argc != 2) {

      error("%s: 引数の数が無効です", argv[0]);
   }

   Token *token = tokenize(argv[1]);
   Node *node   = parse(token);
   codegen(node);

   return 0;
}

