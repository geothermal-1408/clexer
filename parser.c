#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

static AstNode *parse_func_def(Parser *p);
static AstNode *parse_block(Parser *p);
static AstNode *parse_statement(Parser *p);
static AstNode *parse_var_decl(Parser *p);
static AstNode *parse_return(Parser *p);
static AstNode *parse_if(Parser *p);
static AstNode *parse_expr(Parser *p, int min_bp);
static AstNode *parse_primary(Parser *p);


static void error(Parser *p, const char *msg)
{
  if(p->has_error) return;
  fprintf(stderr, "[ERROR]%zu %zu: %s",
	  p->current.line, p->current.col, msg);

  if(p->current.text_len > 0)
    fprintf(stderr, "(got '%.*s')",
	    (int)p->current.text_len, p->current.text);
  
  fprintf(stderr, "\n");
  p->has_error = 1;
}

static void advance(Parser *p)
{
  p->current = lexer_next(&p->lexer);
}

static int match(Parser *p, Token_kind kind)
{
  if(p->current.kind == kind) {
    advance(p);
    return 1;
  }

  return 0;
}

static Token expect(Parser *p, Token_kind kind, const char *ctx)
{
  Token t = p->current;
  if(p->current.kind != kind) {
    char msg[128];
    snprintf(msg, sizeof(msg), "expected token %s in %s", token_kind_str(kind), ctx);
    error(p, msg);
    return t;
  }

  advance(p);
  return t;
}

/** Pratt table **/

typedef struct {
  int left;
  int right;
} BindingPower;

static BindingPower infix_op(Token_kind kind)
{
  switch(kind) {
  case TOKEN_PIPEPIPE: return (BindingPower) {2, 3};
  case TOKEN_AMPAMP: return (BindingPower) {4, 5};
  case TOKEN_EQEQ:
  case TOKEN_BANGEQ: return (BindingPower) {6, 7};
  case TOKEN_GT:   
  case TOKEN_LT:   
  case TOKEN_LTEQ:   
  case TOKEN_GTEQ: return (BindingPower) {8, 9};
  case TOKEN_PLUS: 
  case TOKEN_MINUS: return (BindingPower) {10, 11};
  case TOKEN_STAR:
  case TOKEN_SLASH:
  case TOKEN_PERCENT: return (BindingPower) {12, 13};
  default:
    return (BindingPower) {-1, -1};
  }
}

static AstNode *parse_primary(Parser *p)
{
  Token t = p->current;

  if (t.kind == TOKEN_NUMBER) {
    advance(p);
    return astnode_alloc(NODE_NUMBER, t);
  }

  if(t.kind == TOKEN_SYMBOL) {
    advance(p);
    return astnode_alloc(NODE_SYMBOL, t);
  }

  if(t.kind == TOKEN_LPAREN) {
    advance(p);
    AstNode *inner = parse_expr(p, 0);
    expect(p, TOKEN_RPAREN, "grouped expression");
    return inner;
  }

  if(t.kind == TOKEN_MINUS || t.kind == TOKEN_BANG || t.kind == TOKEN_TILDE) {
    advance(p);
    AstNode *operand = parse_expr(p, 14);
    AstNode *node = astnode_alloc(NODE_UNARY, t);
    node->unary.op = t.kind;
    node->unary.operand = operand;
    return node;
  }

  error(p, "expected an expression");
  advance(p);
  return NULL;
  
}

static AstNode *parse_expr(Parser *p, int min_bp)
{
  AstNode *left = parse_primary(p);
  if(!left) return NULL;

  for(;;) {
    BindingPower bp = infix_op(p->current.kind);
    if(bp.left < min_bp) break;

    Token op = p->current;
    advance(p);

    AstNode *right = parse_expr(p, bp.right);
    if(!right) return NULL;

    AstNode *node	= astnode_alloc(NODE_BINARY, op);
    node->binary.op	= op.kind;
    node->binary.left	= left;
    node->binary.right	= right;
    left = node;
  }
  return left;
}

static AstNode *parse_var_decl(Parser *p)
{
  Token kw = p->current;
  advance(p);

  Token name = expect(p, TOKEN_SYMBOL, "variable declaration");
  AstNode *node = astnode_alloc(NODE_VAR_DECL, kw);
  node->var_decl.name = name;
  node->var_decl.init = NULL;

  if(match(p, TOKEN_EQUALS))
    node->var_decl.init = parse_expr(p, 0);
  expect(p, TOKEN_SEMICOLON, "variable declaration");
  return node;
}

static AstNode *parse_return(Parser *p)
{
  Token kw = p->current;
  advance(p);

  AstNode *node = astnode_alloc(NODE_RETURN, kw);
  node->ret.value = NULL;

  if(p->current.kind != TOKEN_SEMICOLON)
    node->ret.value = parse_expr(p, 0);

  expect(p, TOKEN_SEMICOLON, "return statement");
  return node;
}


static AstNode *parse_if(Parser *p)
{
  Token kw = p->current;
  advance(p);

  expect(p, TOKEN_LPAREN, "if condition");
  AstNode *cond = parse_expr(p, 0);
  expect(p, TOKEN_RPAREN, "if condition");
  AstNode *body = parse_block(p);

  AstNode *node = astnode_alloc(NODE_IF, kw);
  node->if_stmt.condition = cond;
  node->if_stmt.then_block = body;
  return node;
}

static AstNode *parse_statement(Parser *p)
{
  Token t = p->current;

  if(t.kind == TOKEN_KEYWORD && t.kw_kind == KW_INT)
    return parse_var_decl(p);

  if(t.kind == TOKEN_KEYWORD && t.kw_kind == KW_RETURN)
    return parse_return(p);
    
  if(t.kind == TOKEN_KEYWORD && t.kw_kind == KW_IF)
    return parse_if(p);

  if(t.kind == TOKEN_SYMBOL) {
    Lexer saved = p->lexer;
    Token peeked = lexer_next(&p->lexer);

    if(peeked.kind == TOKEN_EQUALS) {
      advance(p);
      advance(p);
      AstNode *val = parse_expr(p, 0);
      expect(p, TOKEN_SEMICOLON, "assignment");
      AstNode *node = astnode_alloc(NODE_ASSIGN, t);
      node->assign.name = t;
      node->assign.value = val;
      return node;
    }
    p->lexer = saved;
  }

  AstNode *expr = parse_expr(p, 0);
  expect(p, TOKEN_SEMICOLON, "expr statement");
  return expr;
}

static AstNode *parse_block(Parser *p)
{
  Token open = expect(p, TOKEN_LBRACE, "block");
  AstNode *node = astnode_alloc(NODE_BLOCK, open);

  while(p->current.kind != TOKEN_RBRACE &&
	p->current.kind != TOKEN_END &&
	!p->has_error) {
    AstNode *stmt = parse_statement(p);
    if(stmt) {
      if(node->block.count >= node->block.capacity) {
	size_t new_cap = node->block.capacity == 0 ? 8 : node->block.capacity;
	node->block.stmts = realloc(node->block.stmts, new_cap * sizeof(AstNode *));
	node->block.capacity = new_cap;
	
      }

      node->block.stmts[node->block.count++] = stmt;
    }
  }

  expect(p, TOKEN_RBRACE, "block");
  return node;
}


static AstNode *parse_func_def(Parser *p)
{
  Token kw = p->current;
  advance(p);

  Token name = expect(p, TOKEN_SYMBOL, "function name");
  AstNode *node = astnode_alloc(NODE_FUNC_DEF, name);
  node->func_def.name = name;

  expect(p, TOKEN_LPAREN, "function parameters");

  while(p->current.kind != TOKEN_RPAREN &&
	p->current.kind != TOKEN_END &&
	!p->has_error) {
    if(!(p->current.kind == TOKEN_KEYWORD &&
	 p->current.kw_kind == KW_INT)) {
      error(p, "expected int");
      break;
    }
    advance(p);

    Token param_name = expect(p, TOKEN_SYMBOL, "parameter name");
    AstNode *param = astnode_alloc(NODE_PARAM, param_name);
    param->param.name = param_name;
    node_list_push(&node->func_def.params, param);
    
    if(!match(p, TOKEN_COMMA)) break;
      
  }

  expect(p, TOKEN_RPAREN, "function parameter");
  node->func_def.body = parse_block(p);

  (void)kw;

  return node;
}

Parser parser_new(const char *src, size_t src_len)
{
  Parser p	= {0};
  p.lexer	= lexer_new(src, src_len);
  p.current	= lexer_next(&p.lexer);
  return p;
}

AstNode *parser_parse(Parser *p)
{
  Token dummy = {0};
  AstNode *root = astnode_alloc(NODE_PROGRAM, dummy);

  while(p->current.kind != TOKEN_END && !p->has_error) {
    if(p->current.kind == TOKEN_KEYWORD && p->current.kw_kind == KW_INT) {
      Lexer saved = p->lexer;
      Token saved_cur = p->current;

      advance(p);
      Token name_tok = p->current;
      advance(p);
      int is_func = (p->current.kind == TOKEN_LPAREN);

      p->lexer = saved;
      p->current = saved_cur;

      (void)name_tok;

      if(is_func) {
	AstNode *fn = parse_func_def(p);
	if(fn) node_list_push(&root->program, fn);
	continue;
      }
    }
  error(p, "excepeted a function defination at top level");
  advance(p);
  }

  if(p->has_error) {
    ast_free(root);
    return NULL;
  }
  return root;
}
