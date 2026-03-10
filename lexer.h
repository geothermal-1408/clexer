#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

typedef enum {
  TOKEN_END = 0,
  TOKEN_INVALID,
  TOKEN_HASH,
  TOKEN_SYMBOL,
  TOKEN_NUMBER,
  TOKEN_STRING,
  TOKEN_EQUALS,
  TOKEN_EQEQ,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_SEMICOLON,
  TOKEN_COMMA,
  TOKEN_LT,
  TOKEN_GT,
  TOKEN_DOT,

} Token_kind;

typedef struct {
  Token_kind kind;
  const char *text;
  size_t text_len;
  unsigned long hash;
} Token;

typedef struct {
  const char *content;
  size_t content_len;
  size_t cursor;
  size_t line;
  size_t bol; //begining of line
} Lexer;


Lexer lexer_new(const char *content, size_t content_len);
Token lexer_next(Lexer *l);

#endif //LEXER_H
