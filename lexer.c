#include <ctype.h>
#include <string.h>

#include "lexer.h"

#define KEYWORD_TABLE_SIZE 64

typedef struct {
  const char *text;
  size_t len;
  int kw_kind;
} KeywordEntry;

static KeywordEntry keyword_table[KEYWORD_TABLE_SIZE];
static int keyword_table_init = 0;

static unsigned long hash_string(const char *str, size_t len)
{
  unsigned long hash = 5381;
  for(size_t i = 0; i < len; ++i) {
    hash = ((hash << 5) + hash) + str[i];
  }
  return hash;
}

static void insert_keyword(const char *text, int kw_kind)
{
  size_t len = strlen(text);
  unsigned long hash = hash_string(text, len);
  size_t index = hash % KEYWORD_TABLE_SIZE;

  //using linear probing
  while(keyword_table[index].text != NULL) {
    index = (index + 1) % KEYWORD_TABLE_SIZE;
  }
  keyword_table[index].text = text;
  keyword_table[index].len = len;
  keyword_table[index].kw_kind = kw_kind;
}

static void init_keyword_table(void)
{
  if(keyword_table_init) return;

  insert_keyword("int", KW_INT);
  insert_keyword("return", KW_RETURN);
  insert_keyword("if", KW_IF);

  keyword_table_init = 1;
}

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

static void skip_whitespace_commments(Lexer *l)
{
  while(l->cursor < l->content_len) {
    char c = peek(l);

    if(isspace((unsigned char)c)) {
      advance(l);
      continue;
    }

    if(c == '/' && l->cursor + 1 < l->content_len) {
      char next_c = l->content[l->cursor + 1];

      if(next_c == '/') {
	while(l->cursor < l->content_len && peek(l) != '\n') {
	  advance(l);
	}

	continue;
      }

      if(next_c == '*') {
	advance(l);
	advance(l);

	while(l->cursor < l->content_len) {
	  if(peek(l) == '*' && l->cursor + 1 < l->content_len && l->content[l->cursor + 1] == '/') {
	    advance(l);
	    advance(l);
	    break;
	  }
	  advance(l);
	}
	continue;
      }
    }

    break;
  }
}

static void check_keyword(Token *token)
{
  token->kw_kind = KW_NONE;

  init_keyword_table();

  size_t index = token->hash % KEYWORD_TABLE_SIZE;
  
  while(keyword_table[index].text != NULL){
    if(keyword_table[index].len == token->text_len &&
       strncmp(keyword_table[index].text, token->text, token->text_len) ==0) {
      token->kind = TOKEN_KEYWORD;
      token->kw_kind = keyword_table[index].kw_kind;
      return;
    }
    index = (index + 1) % KEYWORD_TABLE_SIZE;
  }
}

Lexer lexer_new(const char *content, size_t content_len)
{
  Lexer l = {0};
  l.content = content;
  l.content_len = content_len;
  l.line = 1;
  return l;
}

/* TODO: use hash array for O(1) */
Token lexer_next(Lexer *l)
{
  skip_whitespace_commments(l);
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
    check_keyword(&token);
    return token;
  }

  token.kind = TOKEN_INVALID;
  token.text_len = 1;
  advance(l);
  return token;
}
