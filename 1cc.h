// 1cc.h

#include <assert.h>
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

void error(char *fmt, ...);
void error_token(Token *token, char *fmt, ...);
bool equal(Token *token, char *sign);
Token *skip(Token *token, char *sign);
Token *tokenize(char *argument);

//
// Parser
//

typedef enum {

   ND_ADD,         // +
   ND_SUB,         // -
   ND_MUL,         // *
   ND_DIV,         // /
   ND_EQ,          // ==
   ND_NE,          // !=
   ND_LT,          // <
   ND_LE,          // <=
   ND_RETURN,      // "return"
   ND_EXPR_STMT,   // 式ステートメント
   ND_NUM,         // 整数

} NodeKind;

// AST ノードタイプ
typedef struct Node Node;
struct Node {

   NodeKind kind;  // ノードの種類
   Node    *next;  // 次のノード
   Node     *lhs;  // 左側(Left-hand side)
   Node     *rhs;  // 右側(Right-hand side)
   long      val;  // 種類== ND_NUM の場合に使用
};

Node *parse(Token *token);

//
// codegen.c
//

void codegen(Node *node);

