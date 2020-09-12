// codegen.c

#include "1cc.h"

static int top;

static char *re_gister(int index) {

   static char *r[] = {"%r10", "%r11", "%r12", "%r13", "%r14", "%r15"};

   if (index < 0 || sizeof(r) / sizeof(*r) <= index) {

      error("レジスタが範囲外です: %d", index);
   }

   return r[index];
}

static void gen_expr(Node *node) {

   if (node->kind == ND_NUM) {

      printf("   mov $%lu, %s\n", node->val, re_gister(top++));

      return;
   }

   gen_expr(node->lhs);
   gen_expr(node->rhs);

   char *rd = re_gister(top - 2);
   char *rs = re_gister(top - 1);
   top--;

   switch (node->kind) {

      case ND_ADD:

         printf("   add %s, %s\n", rs, rd);
         return;

      case ND_SUB:

         printf("   sub %s, %s\n", rs, rd);
         return;

      case ND_MUL:

         printf("   imul %s, %s\n", rs, rd);
         return;

      case ND_DIV:

         printf("   mov %s, %%rax\n",   rd);
         printf("   cqo\n"                );
         printf("   idiv %s\n",         rs);
         printf("   mov %%rax, %s\n",   rd);
         return;

      case ND_EQ:

         printf("   cmp %s, %s\n", rs,  rd);
         printf("   sete %%al\n");
         printf("   movzb %%al, %s\n",  rd);
         return;

      case ND_NE:

         printf("   cmp %s, %s\n", rs,  rd);
         printf("   setne %%al\n");
         printf("   movzb %%al, %s\n",  rd);
         return;

      case ND_LT:

         printf("   cmp %s, %s\n", rs,  rd);
         printf("   setl %%al\n");
         printf("   movzb %%al, %s\n",  rd);
         return;

      case ND_LE:

         printf("   cmp %s, %s\n", rs,  rd);
         printf("   setle %%al\n");
         printf("   movzb %%al, %s\n",  rd);
         return;

      default:

         error("無効な式です");
   }
}

static void gen_stmt(Node *node) {

   switch (node->kind) {

      case ND_EXPR_STMT:

         gen_expr(node->lhs);
         printf("  mov %s, %%rax\n", re_gister(--top));
         return;

   default:

      error("invalid statement");
   }
}

void codegen(Node *node) {

   printf(".globl main\n");
   printf("main:\n");

   // 呼び出し先が保存したレジスタを保存します
   printf("   push %%r12\n");
   printf("   push %%r13\n");
   printf("   push %%r14\n");
   printf("   push %%r15\n");

   for (Node *n = node; n; n = n->next) {

      gen_stmt(n);
      assert(top == 0);
   }

   printf("   pop %%r15\n");
   printf("   pop %%r14\n");
   printf("   pop %%r13\n");
   printf("   pop %%r12\n");
   printf("   ret\n");

}

