#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "ast.h"

AstNode *astnode_alloc(NodeKind kind, Token token)
{
  AstNode *n = calloc(1, sizeof(AstNode));
  assert(n != NULL);
  n->kind = kind;
  n->token = token;
  return n;  
}

void node_list_push(NodeList *list, AstNode *node)
{
  if(list->count >= list->capacity) {
    size_t new_cap = list->capacity == 0 ? 8 : list->capacity * 2;
    list->items = realloc(list->items, new_cap * sizeof(AstNode*));

    if(!list->items) {
      fprintf(stderr, "buy more RAM lol!!\n");
      exit(1);
    }
    
    list->capacity = new_cap;
  }
  list->items[list->count++] = node; 
}

void ast_free(AstNode *node)
{
  if(!node) return;

  switch(node->kind) {
    
  case NODE_PROGRAM:
    for(size_t i = 0; i < node->program.count; ++i) {
      ast_free(node->program.items[i]);
    }
    free(node->program.items);
    break;

  case NODE_FUNC_DEF:
    for(size_t i = 0; i < node->func_def.params.count; ++i) {
      ast_free(node->func_def.params.items[i]);
    }
    free(node->func_def.params.items);
    ast_free(node->func_def.body);
    break;
    
  case NODE_CALL:
    for(size_t i = 0; i < node->call.args.count; ++i) {
      ast_free(node->call.args.items[i]);
    }
    free(node->call.args.items);
    break;
    
  case NODE_BLOCK:
    for(size_t i = 0; i < node->block.count; ++i) {
      ast_free(node->block.stmts[i]);
    }
    free(node->block.stmts);
    break;

  case NODE_VAR_DECL:
    ast_free(node->var_decl.init);
    break;

  case NODE_RETURN:
    ast_free(node->ret.value);
    break;
    
  case NODE_STRING:
  case NODE_CHAR_LIT:
    break;
    
  case NODE_IF:
    ast_free(node->if_stmt.condition);
    ast_free(node->if_stmt.then_block);
    ast_free(node->if_stmt.else_block); //NULL safe
    break;
    
  case NODE_WHILE:
    ast_free(node->while_stmt.condition);
    ast_free(node->while_stmt.body);
    break;

  case NODE_ASSIGN:
    ast_free(node->assign.value);
    break;

  case NODE_BINARY:
    ast_free(node->binary.left);
    ast_free(node->binary.right);
    break;

  case NODE_UNARY:
    ast_free(node->unary.operand);
    break;

  case NODE_PARAM:
  case NODE_NUMBER:    
  case NODE_SYMBOL:
    break;
  }
  free(node); 
}

static void pretty_print(int depth)
{
  for(int i = 0; i < depth; ++i) printf(" ");
}

const char *type_kind_str(TypeKind tk)
{
  switch(tk) {
  case TYPE_INT: return "int";
  case TYPE_CHAR: return "char";
  case TYPE_VOID: return "void";    
  }
  return "?";
}

static const char *map_node_str(NodeKind n)
{
  switch(n) {
        case NODE_PROGRAM:  return "Program";
        case NODE_FUNC_DEF: return "FuncDef";
        case NODE_PARAM:    return "Param";
        case NODE_CALL:     return "Call";
        case NODE_BLOCK:    return "Block";
        case NODE_VAR_DECL: return "VarDecl";
        case NODE_RETURN:   return "Return";
        case NODE_IF:       return "If";
        case NODE_WHILE:    return "While";
        case NODE_ASSIGN:   return "Assign";
        case NODE_BINARY:   return "Binary";
        case NODE_UNARY:    return "Unary";
        case NODE_NUMBER:   return "Number";
        case NODE_STRING:   return "String";
        case NODE_CHAR_LIT: return "CharLit";
        case NODE_SYMBOL:   return "Symbol";
    }
}

static const char *map_op_str(Token_kind n)
{
  switch(n) {
        case TOKEN_PLUS:    return "+";
        case TOKEN_MINUS:   return "-";
        case TOKEN_STAR:    return "*";
        case TOKEN_SLASH:   return "/";
        case TOKEN_PERCENT: return "%";
        case TOKEN_EQEQ:    return "==";
        case TOKEN_BANGEQ:  return "!=";
        case TOKEN_LT:      return "<";
        case TOKEN_GT:      return ">";
        case TOKEN_LTEQ:    return "<=";
        case TOKEN_GTEQ:    return ">=";
        case TOKEN_AMPAMP:  return "&&";
        case TOKEN_PIPEPIPE:return "||";
        case TOKEN_BANG:    return "!";
        case TOKEN_TILDE:   return "~";
        default:            return "?";
    }
}

void ast_print(const AstNode *node, int depth)
{
  if(!node) return;

  pretty_print(depth);

  switch(node->kind) {
  case NODE_PROGRAM:
    printf("Program (%zu decls)\n", node->program.count);
    for(size_t i = 0; i < node->program.count; ++i) {
      ast_print(node->program.items[i], depth+1);
    }
    break;

  case NODE_FUNC_DEF:
    printf("FuncDef '%.*s' (%zu params)\n", (int)node->token.text_len, node->token.text, node->func_def.params.count);
    for(size_t i = 0; i < node->func_def.params.count; ++i) {
      ast_print(node->func_def.params.items[i], depth+1);
    }
    ast_print(node->func_def.body,depth + 1);
    break;

  case NODE_CALL:
    printf("Call '%.*s' (%zu args)\n",(int)node->call.callee.text_len, node->call.callee.text, node->call.args.count);
    for(size_t i = 0; i < node->call.args.count; ++i)
      ast_print(node->call.args.items[i], depth + 1);
    break;
    
  case NODE_PARAM:
    printf("Param '%.*s'\n", (int)node->token.text_len, node->token.text);
    break;

  case NODE_BLOCK:
    printf("Block (%zu stmts)\n", node->block.count);
    for(size_t i = 0; i < node->block.count; ++i) {
      ast_print(node->block.stmts[i], depth + 1);
    }
    break;
    
  case NODE_VAR_DECL:
    printf("VarDecl '%.*s'%s\n",
	   (int)node->var_decl.name.text_len,
	   node->var_decl.name.text,
	   node->var_decl.init ? " =" : "");
    if(node->var_decl.init)
      ast_print(node->var_decl.init, depth + 1);
    break;

  case NODE_RETURN:
    printf("Return\n");
    ast_print(node->ret.value, depth + 1);
    break;

  case NODE_STRING:
    printf("String \"%.*s\"\n", (int)node->token.text_len, node->token.text);
    break;
    
  case NODE_CHAR_LIT:
    printf("CharLit '%.*s'\n", (int)node->token.text_len, node->token.text);
    break;

  case NODE_IF:
    printf("If\n");
    pretty_print(depth + 1); printf("Condition:\n");
    ast_print(node->if_stmt.condition, depth + 2);
    pretty_print(depth + 1); printf("then:\n");
    ast_print(node->if_stmt.then_block, depth + 2);
    if(node->if_stmt.else_block) {
      pretty_print(depth + 1);
      printf("else:\n");
      ast_print(node->if_stmt.else_block, depth + 2);
    }
    
    break;

  case NODE_WHILE:
    printf("While\n");
    pretty_print(depth + 1); printf("Condition:\n");
    ast_print(node->while_stmt.condition, depth + 2);
    pretty_print(depth + 1); printf("body:\n");
    ast_print(node->while_stmt.body, depth + 2);
    break;

  case NODE_ASSIGN:
    printf("Assign '%.*s'\n",
	   (int)node->assign.name.text_len, node->assign.name.text);
    ast_print(node->assign.value, depth + 1);
    break;

  case NODE_BINARY:
    printf("Binary '%s'\n", map_op_str(node->binary.op));
    ast_print(node->binary.left, depth + 1);
    ast_print(node->binary.right, depth + 1);
    break;

  case NODE_UNARY:
    printf("Unary '%s'\n", map_op_str(node->unary.op));
    ast_print(node->unary.operand, depth + 1);
    break;

  case NODE_NUMBER:
    printf("Number '%.*s'\n",
	   (int)node->token.text_len, node->token.text);
    break;

  case NODE_SYMBOL:
    printf("Symbol '%.*s'\n",
	   (int)node->token.text_len, node->token.text);
    break;
    
  default:
    printf("<%s>\n", map_node_str(node->kind));
    break;
  }
}
