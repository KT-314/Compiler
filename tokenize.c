// tokenize.c

#include "1cc.h"

// 入力文字列
static char *current_input;

// エラーを報告して終了します
void error(char *fmt, ...) {

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

void error_token(Token *token, char *fmt, ...) {

   va_list ap;
   va_start(ap, fmt);
   verror_at(token->location, fmt, ap);
}

// `sign` ( 記号) に一致する場合、現在のトークンを消費します
bool equal(Token *token, char *sign) {

   return strlen(sign) == token->length &&
         !strncmp(token->location, sign, token->length);
}

// 現在のトークンが `sign` ( 記号) であることを確認します
Token *skip(Token *token, char *sign) {

   if (!equal(token, sign)) {

      error_token(token, "予期されるトークン '%s'", sign);
   }

   return token->next;
}

// 新しいトークンを作成し`cur` の次のトークンとして追加します
static Token *new_token(TokenKind kind, Token *cur, char *str, int len) {

   Token *token    = calloc(1, sizeof(Token));
   token->kind     = kind;
   token->location = str;
   token->length   = len;
   cur->next       = token;

   return token;
}

static bool startswith(char *p, char *q) {

   return strncmp(p, q, strlen(q)) == 0;
}

static bool is_alpha(char c) {

   return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static bool is_alnum(char c) {

   return is_alpha(c) || ('0' <= c && c <= '9');
}

// `current_input` をトークン化し、新しいトークンを返します
Token *tokenize(char *argument) {

   current_input  = argument;

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

      // キーワード
      if (startswith( argument, "return") && !is_alnum(argument[6])) {

         current = new_token(TK_RESERVED, current, argument, 6);
         argument += 6;
         continue;
      }

      // 識別子
      if (is_alpha(*argument)) {

         char *q = argument++;

         while (is_alnum(*argument))
         argument++;
         current = new_token(TK_IDENT, current, q, argument - q);
         continue;
      }

      // 複数文字の句読点
      if (startswith(argument, "==") || startswith(argument, "!=") ||
          startswith(argument, "<=") || startswith(argument, ">=")) {

         current = new_token(TK_RESERVED, current, argument, 2);

         argument += 2;
         continue;
      }

      // 1文字の句読点
      if (ispunct(*argument)) {

         current = new_token(TK_RESERVED, current, argument++, 1);
         continue;
      }

      error_at(argument, "無効なトークンです");
   }

   new_token(TK_EOF, current, argument, 0);

   return head.next;
}
