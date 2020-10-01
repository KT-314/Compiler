// parse.c

#include "1cc.h"

static Node *expr(Token **rest, Token *token);
static Node *expr_stmt(Token **rest, Token *token);
static Node *assign(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *token);
static Node *relational(Token **rest, Token *token);
static Node *add(Token **rest, Token *token);
static Node *mul(Token **rest, Token *token);
static Node *unary(Token **rest, Token *token);
static Node *primary(Token **rest, Token *token);

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

static Node *new_unary(NodeKind kind, Node *expr) {

   Node *node = new_node(kind);
   node->lhs  = expr;

   return node;
}

static Node *new_num(long val) {

   Node *node = new_node(ND_NUM);
   node->val  = val;

   return node;
}

static Node *new_var_node(char name) {

   Node *node = new_node(ND_VAR);
   node->name = name;

   return node;
}

// 現在のトークンがTK_NUMERIC(数値リテラル) であることを確認します
static long get_number(Token *token) {

   if (token->kind != TK_NUMERIC) {

      error_token(token, "数値が必要です");
   }

   return token->value;
}

//           stmt = "return" expr ";"
//                |  expr-stmt
static Node *stmt(Token **rest, Token *token) {

   if (equal(token, "return")) {

      Node *node = new_unary(ND_RETURN, expr(&token, token->next));
      *rest = skip(token, ";");
      return node;
   }

   return expr_stmt(rest, token);
}

//           expr-stmt = expr ";"
static Node *expr_stmt(Token **rest, Token *token) {

   Node *node = new_unary(ND_EXPR_STMT, expr(&token, token));
   *rest = skip(token, ";");

   return node;
}

//           expr = assign
static Node *expr(Token **rest, Token *token) {

   return assign(rest, token);
}

//           assign = equality ("=" assign)?
static Node *assign(Token **rest, Token *token) {

   Node *node = equality(&token, token);

   if (equal(token, "="))

      node  = new_binary(ND_ASSIGN, node, assign(&token, token->next));
      *rest = token;
      return node;
   }

//           equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **rest, Token *token) {

   Node *node = relational(&token, token);

   for (;;) {

      if (equal(token, "==")) {

         Node *rhs = relational(&token, token->next);
         node = new_binary(ND_EQ, node, rhs);
         continue;
      }

      if (equal(token, "!=")) {

         Node *rhs = relational(&token, token->next);
         node = new_binary(ND_NE, node, rhs);
         continue;
      }

      *rest = token;

      return node;
   }
}

//           relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **rest, Token *token) {

   Node *node = add(&token, token);

   for (;;) {

      if (equal(token, "<")) {

         Node *rhs = add(&token, token->next);
         node = new_binary(ND_LT, node, rhs);
         continue;
      }

      if (equal(token, "<=")) {

         Node *rhs = add(&token, token->next);
         node = new_binary(ND_LE, node, rhs);
         continue;
      }

      if (equal(token, ">")) {

         Node *rhs = add(&token, token->next);
         node = new_binary(ND_LT, rhs, node);
         continue;
      }

      if (equal(token, ">=")) {

         Node *rhs = add(&token, token->next);
         node = new_binary(ND_LE, rhs, node);
         continue;
      }

      *rest = token;

      return node;
   }
}

//           add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *token) {

   Node *node = mul(&token, token);

   for (;;) {

      if (equal(token, "+")) {

         Node *rhs = mul(&token, token->next);
         node = new_binary(ND_ADD, node, rhs);
         continue;
      }

      if (equal(token, "-")) {

         Node *rhs = mul(&token, token->next);
         node = new_binary(ND_SUB, node, rhs);
         continue;
      }

     *rest = token;

     return node;
   }
}

//           mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **rest, Token *token) {

   Node *node = unary(&token, token);

   for (;;) {

      if (equal(token, "*")) {

         Node *rhs = unary(&token, token->next);
         node = new_binary(ND_MUL, node, rhs);
         continue;
      }

      if (equal(token, "/")) {

         Node *rhs = unary(&token, token->next);
         node = new_binary(ND_DIV, node, rhs);
         continue;
      }

      *rest = token;

      return node;
   }
}

//           unary = ("+" | "-") unary | primary
static Node *unary(Token **rest, Token *token) {

   if (equal(token, "+")) {

      return unary(rest, token->next);
   }

   if (equal(token, "-")) {

      return new_binary(ND_SUB, new_num(0), unary(rest, token->next));
   }

   return primary(rest, token);
}

//           primary = "(" expr ")" | ident | num
static Node *primary(Token **rest, Token *token) {

   if (equal(token, "(")) {

      Node *node = expr(&token, token->next);
      *rest = skip(token, ")");

      return node;
   }

   Node *node;

   if (token->kind == TK_IDENT)

      node = new_var_node(*token->location);

   else
      node = new_num(get_number(token));

   *rest = token->next;

   return node;
}

//    program = stmt*
Node *parse(Token *token) {

   Node head = {};
   Node *cur = &head;

   while (token->kind != TK_EOF) {

   cur = cur->next = stmt(&token, token);
   }

   return head.next;
}

