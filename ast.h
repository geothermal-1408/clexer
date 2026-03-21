#ifndef AST_H
#define AST_H

#include <stddef.h>
#include "lexer.h"

typedef enum {
  TYPE_INT,
  TYPE_CHAR,
  TYPE_VOID,
}TypeKind;

typedef struct {
  TypeKind kind;
  int pointer_level;
  Token token;
} Type;


typedef enum {
  NODE_PROGRAM,
  NODE_FUNC_DEF,
  NODE_PARAM,
  NODE_BLOCK,
  NODE_VAR_DECL,
  NODE_RETURN,
  NODE_IF,
  NODE_WHILE,
  NODE_ASSIGN,
  NODE_BINARY,
  NODE_UNARY,
  NODE_CALL,
  NODE_NUMBER,
  NODE_STRING,
  NODE_CHAR_LIT,
  NODE_SYMBOL,
  NODE_SUBSCRIPT,
} NodeKind;

typedef struct AstNode AstNode;

/*-- Node Payloads --*/

typedef struct {
  AstNode **items;
  size_t    count;
  size_t    capacity;
} NodeList;

typedef struct {
  Type return_type;
  Token name;
  NodeList params;
  AstNode *body;
} FuncDef;

typedef struct {
  Type type;
  Token name;
} Param;

typedef struct {
  AstNode **stmts;
  size_t count;
  size_t capacity;
} Block;

typedef struct {
  Type     type;
  Token    name;
  AstNode *init;
} VarDecl;

typedef struct {
  AstNode *value;
} Return;

typedef struct {
  AstNode *condition;
  AstNode *then_block;
  AstNode *else_block;
} If;

typedef struct {
  AstNode *condition;
  AstNode *body;
} While;

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

typedef struct {
  AstNode *array;
  AstNode *index;
} Subsctipt;
  
typedef struct {
  Token callee;
  NodeList args;
} Call;

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
    While while_stmt;
    Assign assign;
    Binary binary;
    Unary unary;
    Subsctipt subscript;
    Call call;
  };
};

AstNode *astnode_alloc(NodeKind kind, Token token);
void node_list_push(NodeList *list, AstNode *node);
void ast_free(AstNode *node);
void ast_print(const AstNode *node, int indent);
const char *type_kind_str(TypeKind kind);

#endif //AST_H
