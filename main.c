// main.c

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// Tokenizer
//

typedef enum {

   TK_RESERVED,    // キーワードまたは句読点
   TK_NUMERIC,     // 数値リテラル
   TK_EOF,         // ファイルの終わりマーカー

} TokenKind;

// トークンのタイプ
typedef struct Token Token;
struct Token {

   TokenKind kind; // トークンの種類
   Token    *next; // 次のトークン
   long     value; // 種類がTK_NUMERIC の場合、その値
   char *location; // トークンの場所
   int     length; // トークンの長さ
};

// 入力文字列
static char *current_input;

// エラーを報告して終了します
static void error(char *fmt, ...) {

   va_list ap;
   va_start(ap, fmt);
   vfprintf(stderr, fmt, ap);
   fprintf(stderr, "\n");
   exit(1);
}

// エラーの場所を報告して終了します
static void verror_at(char *loc, char *fmt, va_list ap) {

   int pos = loc - current_input;
   fprintf(stderr, "%s\n", current_input);
   fprintf(stderr, "%*s", pos, ""); // pos スペースを出力します
   fprintf(stderr, "^___ ");
   vfprintf(stderr, fmt, ap);
   fprintf(stderr, "\n");
   exit(1);
}

static void error_at(char *loc, char *fmt, ...) {

   va_list ap;
   va_start(ap, fmt);
   verror_at(loc, fmt, ap);
}

static void error_tok(Token *tok, char *fmt, ...) {

   va_list ap;
   va_start(ap, fmt);
   verror_at(tok->location, fmt, ap);
}

// `sign` ( 記号) に一致する場合、現在のトークンを消費します
static bool equal(Token *tok, char *sign) {

   return strlen(sign) == tok->length &&
         !strncmp(tok->location, sign, tok->length);
}

// 現在のトークンが `sign` ( 記号) であることを確認します
static Token *skip(Token *tok, char *sign) {

   if (!equal(tok, sign))
      error_tok(tok, "予期されるトークン '%s'", sign);
   return tok->next;
}

// 現在のトークンがTK_NUMERIC(数値リテラル) であることを確認します
static long get_number(Token *tok) {

   if (tok->kind != TK_NUMERIC)
      error_tok(tok, "数値が必要です");
   return tok->value;
}

// 新しいトークンを作成し`cur` の次のトークンとして追加します
static Token *new_token(TokenKind kind, Token *cur, char *str, int len) {

   Token *tok    = calloc(1, sizeof(Token));
   tok->kind     = kind;
   tok->location = str;
   tok->length   = len;
   cur->next     = tok;
   return tok;
}

// `current_input` をトークン化し、新しいトークンを返します
static Token *tokenize(void) {

   char *argument = current_input;
   Token     head = {};
   Token *current = &head;

   while (*argument) {

      // 空白文字をスキップします
      if (isspace(*argument)) {

         argument++;
         continue;
      }

      // 数値リテラル
      if (isdigit(*argument)) {

         current         = new_token(TK_NUMERIC, current, argument, 0);
         char      *now  = argument;
         current->value  = strtoul(argument, &argument, 10);
         current->length = argument - now;
         continue;
      }

      // 句読点
      if (ispunct(*argument)) {

         current = new_token(TK_RESERVED, current, argument++, 1);
         continue;
      }

      error_at(argument, "無効なトークンです");
   }

   new_token(TK_EOF, current, argument, 0);
   return head.next;
}

//
// Parser
//

typedef enum {

   ND_ADD,         // +
   ND_SUB,         // -
   ND_MUL,         // *
   ND_DIV,         // /
   ND_NUM,         // 整数

} NodeKind;

// AST ノードタイプ
typedef struct Node Node;
struct Node {

   NodeKind kind;  // ノードの種類
   Node     *lhs;  // 左側
   Node     *rhs;  // 右側
   long      val;  // 種類== ND_NUM の場合に使用
};

static Node *new_node(NodeKind kind) {

   Node *node = calloc(1, sizeof(Node));
   node->kind = kind;
   return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {

   Node *node = new_node(kind);
   node->lhs  = lhs;
   node->rhs  = rhs;
   return node;
}

static Node *new_num(long val) {

   Node *node = new_node(ND_NUM);
   node->val  = val;
   return node;
}

static Node *expr(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

//           expr = mul ("+" mul | "-" mul)*
static Node *expr(Token **rest, Token *tok) {

   Node *node = mul(&tok, tok);

   for (;;) {

      if (equal(tok, "+")) {

         Node *rhs = mul(&tok, tok->next);
         node = new_binary(ND_ADD, node, rhs);
         continue;
      }

      if (equal(tok, "-")) {

         Node *rhs = mul(&tok, tok->next);
         node = new_binary(ND_SUB, node, rhs);
         continue;
      }

     *rest = tok;
     return node;
   }
}

//           mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **rest, Token *tok) {

   Node *node = unary(&tok, tok);

   for (;;) {

      if (equal(tok, "*")) {

         Node *rhs = unary(&tok, tok->next);
         node = new_binary(ND_MUL, node, rhs);
         continue;
      }

      if (equal(tok, "/")) {

         Node *rhs = unary(&tok, tok->next);
         node = new_binary(ND_DIV, node, rhs);
         continue;
      }

      *rest = tok;
      return node;
   }
}

//           unary = ("+" | "-") unary | primary
static Node *unary(Token **rest, Token *tok) {

   if (equal(tok, "+"))

      return unary(rest, tok->next);

   if (equal(tok, "-"))

      return new_binary(ND_SUB, new_num(0), unary(rest, tok->next));

   return primary(rest, tok);
}

//           primary = "(" expr ")" | num
static Node *primary(Token **rest, Token *tok) {

   if (equal(tok, "(")) {

      Node *node = expr(&tok, tok->next);
      *rest = skip(tok, ")");
      return node;
   }

  Node *node = new_num(get_number(tok));
  *rest = tok->next;
  return node;
}

//
// Code generator
//

static char *reg(int idx) {

   static char *r[] = {"%r10", "%r11", "%r12", "%r13", "%r14", "%r15"};

   if (idx < 0 || sizeof(r) / sizeof(*r) <= idx)
      error("register out of range: %d", idx);
      return r[idx];
}

static int top;

static void gen_expr(Node *node) {

   if (node->kind == ND_NUM) {

      printf("  mov $%lu, %s\n", node->val, reg(top++));
      return;
   }

   gen_expr(node->lhs);
   gen_expr(node->rhs);

   char *rd = reg(top - 2);
   char *rs = reg(top - 1);
   top--;

   switch (node->kind) {

      case ND_ADD:
         printf("  add %s, %s\n", rs, rd);
         return;
      case ND_SUB:
         printf("  sub %s, %s\n", rs, rd);
         return;
      case ND_MUL:
         printf("  imul %s, %s\n", rs, rd);
         return;
      case ND_DIV:
         printf("  mov %s, %%rax\n", rd);
         printf("  cqo\n");
         printf("  idiv %s\n", rs);
         printf("  mov %%rax, %s\n", rd);
         return;
      default:
         error("invalid expression");
   }
}

int main(int argc, char **argv) {

   if (argc != 2)

      error("%s: 引数の数が無効です", argv[0]);

   // トークン化して解析します
   current_input = argv[1];
   Token *tok = tokenize();
   Node *node = expr(&tok, tok);

   if (tok->kind != TK_EOF)

      error_tok(tok, "extra token");

   printf(".globl main\n");
   printf("main:\n");

   // 呼び出し先が保存したレジスタを保存します
   printf("  push %%r12\n");
   printf("  push %%r13\n");
   printf("  push %%r14\n");
   printf("  push %%r15\n");

   // AST をトラバースしてアセンブリを出力します
   gen_expr(node);

   // 式の結果をRAXに設定して、
   // 結果はこの関数の戻り値になります
   printf("  mov %s, %%rax\n", reg(top - 1));

   printf("  pop %%r15\n");
   printf("  pop %%r14\n");
   printf("  pop %%r13\n");
   printf("  pop %%r12\n");
   printf("  ret\n");
   return 0;
}

