// 1cc.h

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// tokenizer.c
//

// トークン
typedef enum {

   TK_RESERVED,    // キーワードまたは句読点
   TK_IDENT,       // 識別子
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
// parser.c
//

// ローカル変数
typedef struct Var Var;
struct Var {

   Var  *next;
   char *name;     // 変数名
   int offset;     // RBP からのオフセット
};

// AST ノード
typedef enum {

   ND_ADD,         // +
   ND_SUB,         // -
   ND_MUL,         // *
   ND_DIV,         // /
   ND_EQ,          // ==
   ND_NE,          // !=
   ND_LT,          // <
   ND_ASSIGN,      // =
   ND_LE,          // <=
   ND_RETURN,      // "return"
   ND_EXPR_STMT,   // 式ステートメント
   ND_VAR,         // 変数
   ND_NUM,         // 整数

} NodeKind;

// AST ノードタイプ
typedef struct Node Node;
struct Node {

   NodeKind kind;  // ノードの種類
   Node    *next;  // 次のノード
   Node     *lhs;  // 左側(Left-hand side)
   Node     *rhs;  // 右側(Right-hand side)
   Var      *var;  // 種類== ND_VAR の場合に使用
   long      val;  // 種類== ND_NUM の場合に使用
};

typedef struct Function Function;
struct Function {

   Node     *node;
   Var    *locals;
   int stack_size;
};

Function *parse(Token *tok);

//
// codegen.c
//

void codegen(Function *prog);

