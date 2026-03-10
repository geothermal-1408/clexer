#include "lexer.h"
#include <ctype.h>

static char peek(Lexer *l)
{
  if(l->cursor >= l->content_len) {
    return '\0';
  }
  return l->content[l->cursor];
}

static void advance(Lexer *l)
{
  if(l->cursor < l->content_len) {
    if(l->content[l->cursor] == '\n') {
      l->line++;
      l->bol = l->cursor + 1;
    }
    l->cursor++;
  }
}

static void skip_whitespace(Lexer *l)
{
  while(l->cursor < l->content_len && isspace(peek(l))){
    advance(l);
  }
}

static unsigned long hash_string(const char *str, size_t len)
{
  unsigned long hash = 5381;
  for(size_t i = 0; i < len; ++i) {
    hash = ((hash << 5) + hash) + str[i];
  }
  return hash;
}

Lexer lexer_new(const char *content, size_t content_len)
{
  Lexer l = {0};
  l.content = content;
  l.content_len = content_len;
  l.line = 1;
  return l;
}

Token lexer_next(Lexer *l)
{
  skip_whitespace(l);
  Token token = {0};
  token.text = &l->content[l->cursor];

  if(l->cursor >= l->content_len) {
    token.kind = TOKEN_END;
    token.text_len = 0;
    return token;
  }

  char c = peek(l);

  switch (c) {
      case '#': token.kind = TOKEN_HASH;      token.text_len = 1; advance(l); return token;
      case '(': token.kind = TOKEN_LPAREN;    token.text_len = 1; advance(l); return token;
      case ')': token.kind = TOKEN_RPAREN;    token.text_len = 1; advance(l); return token;
      case '{': token.kind = TOKEN_LBRACE;    token.text_len = 1; advance(l); return token;
      case '}': token.kind = TOKEN_RBRACE;    token.text_len = 1; advance(l); return token;
      case ';': token.kind = TOKEN_SEMICOLON; token.text_len = 1; advance(l); return token;
      case ',': token.kind = TOKEN_COMMA;     token.text_len = 1; advance(l); return token;
      case '<': token.kind = TOKEN_LT;        token.text_len = 1; advance(l); return token;
      case '>': token.kind = TOKEN_GT;        token.text_len = 1; advance(l); return token;
      case '.': token.kind = TOKEN_DOT;       token.text_len = 1; advance(l); return token;
      case '=': 
          advance(l); 
          if (peek(l) == '=') {
              advance(l); 
              token.kind = TOKEN_EQEQ;
              token.text_len = 2;
          } else {
              token.kind = TOKEN_EQUALS;
              token.text_len = 1;
          }
          return token;
      
  }

  if(isdigit((unsigned char) c)) {
    token.kind = TOKEN_NUMBER;
    size_t start_cursor = l->cursor;

    while(isdigit((unsigned char) peek(l))) {
      advance(l);
    }

    token.text_len = l->cursor - start_cursor;
    return token;
  }

  if(c == '"') {
    token.kind = TOKEN_STRING;
    advance(l);

    size_t start_cursor = l->cursor;

    while(peek(l) != '"' && peek(l) != '\0') {
      advance(l);
    }

    token.text_len = l->cursor - start_cursor;
    token.text = &l->content[start_cursor];

    if(peek(l) == '"') {
      advance(l);
    }
    return token;
  }


  if(isalpha((unsigned char)c) || c == '_') {
    token.kind = TOKEN_SYMBOL;
    size_t start_cursor = l->cursor;

    while(isalnum((unsigned char)peek(l)) || peek(l) == '_'){
      advance(l);
    }
    token.text_len = l->cursor - start_cursor;
    token.hash = hash_string(token.text, token.text_len);
    return token;
  }

  token.kind = TOKEN_INVALID;
  token.text_len = 1;
  advance(l);
  return token;
}
