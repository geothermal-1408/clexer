#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
  Lexer lexer;
  Token current; 
  int has_error;
} Parser;

Parser   parser_new(const char *src, size_t src_len);

AstNode *parser_parse(Parser *parser);
  
#endif // PARSER_H
