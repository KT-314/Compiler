// main.c

#include "1cc.h"

// `n` を`align` の最も近い倍数に切り上げます。例えば、
// align_to(5,8) は8 を返し、align_to(11, 8) は16 を返します
static int align_to(int n, int align) {

   return (n + align - 1) / align * align;
}

int main(int argc, char **argv) {

   if (argc != 2) {

      error("%s: 引数の数が無効です", argv[0]);
   }

   Token *token = tokenize(argv[1]);
   Function *prog = parse(token);

   // ローカル変数にオフセットを割り当てます
   int offset = 32;  // 呼び出し先が保存したレジスタの場合は32
   for (Var *var = prog->locals; var; var = var->next) {

      offset += 8;
      var->offset = offset;
   }

   prog->stack_size = align_to(offset, 16);

   // AST をトラバースしてアセンブリを発行します
   codegen(prog);

   return 0;
}

