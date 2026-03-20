#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include "pp.h"

typedef struct {
  TokenStream *ts;
  size_t ts_index;
  Lexer lexer;
  int use_stream;
  Token current;
  int has_error;
} Parser;

Parser   parser_new(const char *src, size_t src_len);

AstNode *parser_parse(Parser *parser);

/* add a token-stream-based constructor */
Parser parser_from_stream(TokenStream *ts);
  
#endif // PARSER_H
