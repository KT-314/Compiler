// main.c

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// `s` に一致する場合、現在のトークンを消費します
static bool equal(Token *tok, char *s) {

   return strlen(s) == tok->length &&
         !strncmp(tok->location, s, tok->length);
}

// 現在のトークンが `s` であることを確認します
static Token *skip(Token *tok, char *s) {

   if (!equal(tok, s))
      error_tok(tok, "予期されるトークン '%s'", s);
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

   char *p = current_input;
   Token head = {};
   Token *cur = &head;

   while (*p) {

      // 空白文字をスキップします
      if (isspace(*p)) {
         p++;
         continue;
      }

      // 数値リテラル
      if (isdigit(*p)) {
         cur = new_token(TK_NUMERIC, cur, p, 0);
         char *q = p;
         cur->value  = strtoul(p, &p, 10);
         cur->length = p - q;
         continue;
      }

      // 句読点
      if (*p == '+' || *p == '-') {
         cur = new_token(TK_RESERVED, cur, p++, 1);
         continue;
      }

      error_at(p, "無効なトークンです");
   }

   new_token(TK_EOF, cur, p, 0);
   return head.next;
}

int main(int argc, char **argv) {

   if (argc != 2) 
      error("%s: 引数の数が無効です\n", argv[0]);

   current_input = argv[1];
   Token *tok = tokenize();

   printf(".globl main\n");
   printf("main:\n");

   // 最初のトークンは数値でなければなりません
   printf("  mov $%ld, %%rax\n", get_number(tok));
   tok = tok->next;


loop:

   // ...の後に `+ < number >`または`- < number >` が続きます
   while (tok->kind != TK_EOF) {

      if (equal(tok, "+")) {
         goto add;
      }

      goto sub;
  
add:
   printf("  add $%ld, %%rax\n", get_number(tok->next));
   tok = tok->next->next;
   goto loop;

sub:
   tok = skip(tok, "-");
   printf("  sub $%ld, %%rax\n", get_number(tok));
   tok = tok->next;
   goto loop;

   }

   printf("  ret\n");
   return 0;
}

