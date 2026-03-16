#ifndef AST_H
#define AST_H

#include <stddef.h>
#include "lexer.h"

typedef enum {
  NODE_PROGRAM,
  NODE_FUNC_DEF,
  NODE_PARAM,
  NODE_BLOCK,
  NODE_VAR_DECL,
  NODE_RETURN,
  NODE_IF,
  NODE_ASSIGN,
  NODE_BINARY,
  NODE_UNARY,
  NODE_NUMBER,
  NODE_SYMBOL,
} NodeKind;

typedef struct AstNode AstNode;

/*-- Node Payloads --*/

typedef struct {
  AstNode **items;
  size_t    count;
  size_t    capacity;
} NodeList;

typedef struct {
  Token name;
  NodeList params;
  AstNode *body;
} FuncDef;

typedef struct {
  Token name;
} Param;

typedef struct {
  AstNode **stmts;
  size_t count;
  size_t capacity;
} Block;

typedef struct {
  Token    name;
  AstNode *init;
} VarDecl;

typedef struct {
  AstNode *value;
} Return;

typedef struct {
  AstNode *condition;
  AstNode *then_block;
} If;

typedef struct {
  Token    name;
  AstNode *value;
} Assign;

typedef struct {
  Token_kind op;
  AstNode *left;
  AstNode *right;
} Binary;

typedef struct {
  Token_kind op;
  AstNode *operand;
} Unary;

/*--  --*/

struct AstNode {
  NodeKind kind;
  Token token;

  union {
    NodeList program;
    FuncDef func_def;
    Param param;
    Block block;
    VarDecl var_decl;
    Return ret;
    If if_stmt;
    Assign assign;
    Binary binary;
    Unary unary;
  };
};

AstNode *astnode_alloc(NodeKind kind, Token token);

void node_list_push(NodeList *list, AstNode *node);

void ast_free(AstNode *node);

void ast_print(const AstNode *node, int indent);

#endif //AST_H
