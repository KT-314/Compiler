// parse.c

#include "1cc.h"

// 解析中に作成されるすべてのローカル変数インスタンスは
// このリストに蓄積されます
Var *locals;

static Node *compound_stmt(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *token);
static Node *expr_stmt(Token **rest, Token *token);
static Node *assign(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *token);
static Node *relational(Token **rest, Token *token);
static Node *add(Token **rest, Token *token);
static Node *mul(Token **rest, Token *token);
static Node *unary(Token **rest, Token *token);
static Node *primary(Token **rest, Token *token);

// 名前でローカル変数を検索します
static Var *find_var(Token *token) {

   for (Var *var = locals; var; var = var->next)

      if (strlen(var->name) == token->length &&
         !strncmp(token->location, var->name, token->length))
      return var;

   return NULL;
}

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

static Node *new_var_node(Var *var) {

   Node *node = new_node(ND_VAR);
   node->var  = var;

   return node;
}

static Var *new_lvar(char *name) {

   Var *var  = calloc(1, sizeof(Var));
   var->name = name;
   var->next = locals;
   locals    = var;

   return var;
}

// 現在のトークンがTK_NUMERIC(数値リテラル) であることを確認します
static long get_number(Token *token) {

   if (token->kind != TK_NUMERIC) {

      error_token(token, "数値が必要です");
   }

   return token->value;
}

//           stmt = "return" expr ";"
//                | "if"  "(" expr ")" stmt ("else" stmt)?
//                | "for" "(" expr-stmt expr? ";" expr? ")" stmt
//                | "{" compound-stmt
//                |  expr-stmt
static Node *stmt(Token **rest, Token *token) {

   if (equal(token, "return")) {

      Node *node = new_unary(ND_RETURN, expr(&token, token->next));
      *rest = skip(token, ";");
      return node;
   }

   if (equal(token, "if")) {

      Node *node = new_node(ND_IF);
      token = skip(token->next, "(");
      node->cond = expr(&token, token);
      token = skip(token, ")");
      node->then = stmt(&token, token);

   if (equal(token, "else"))
      node->els = stmt(&token, token->next);
      *rest = token;
   return node;
   }

   if (equal(token, "for")) {

      Node *node = new_node(ND_FOR);
      token = skip(token->next, "(");

      node->init = expr_stmt(&token, token);

      if (!equal(token, ";"))
         node->cond = expr(&token, token);
      token = skip(token, ";");

      if (!equal(token, ")"))
         node->inc = expr(&token, token);
      token = skip(token, ")");

      node->then = stmt(rest, token);
      return node;
   }

   if (equal(token, "{"))

      return compound_stmt(rest, token->next);

   return expr_stmt(rest, token);
}

//           compound-stmt = stmt* "}"
static Node *compound_stmt(Token **rest, Token *token) {

   Node head = {};
   Node *cur = &head;

   while (!equal(token, "}"))

      cur = cur->next = stmt(&token, token);

   Node *node = new_node(ND_BLOCK);
   node->body = head.next;
   *rest = token->next;
   return node;
}


//           expr-stmt = expr? ";"
static Node *expr_stmt(Token **rest, Token *token) {

   if (equal(token, ";")) {

      Node *node = new_node(ND_BLOCK);
      *rest = token->next;
      return node;
   }

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

   if (token->kind == TK_IDENT) {

      Var *var = find_var(token);

   if (!var)

      var = new_lvar(strndup(token->location, token->length));

      *rest = token->next;

      return new_var_node(var);
   }

   Node *node = new_num(get_number(token));
   *rest = token->next;

   return node;
}

//    program = stmt*
Function *parse(Token *token) {

   token = skip(token, "{");

   Function *prog = calloc(1, sizeof(Function));
   prog->body     = compound_stmt(&token, token);
   prog->locals   = locals;

   return prog;
}

