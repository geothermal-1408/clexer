#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

typedef enum {
  KW_NONE = 0,
  KW_INT,
  KW_RETURN, 
  KW_IF,
} Keyword_kind;

typedef enum {
  TOKEN_END = 0,
  TOKEN_INVALID,

  /* literals & identifiers */
  TOKEN_HASH,
  TOKEN_SYMBOL,
  TOKEN_KEYWORD,
  TOKEN_NUMBER,
  TOKEN_STRING,

  /* assignment */
  TOKEN_EQUALS,   
  TOKEN_PLUSEQ,   
  TOKEN_MINUSEQ,  
  TOKEN_STAREQ,   
  TOKEN_SLASHEQ,  
  TOKEN_PERCENTEQ,
  TOKEN_AMPEQ,    
  TOKEN_PIPEEQ,   
  TOKEN_CARETEQ,  
  TOKEN_LSHIFTEQ, 
  TOKEN_RSHIFTEQ, 

  /* arithmetic */
  TOKEN_PLUS,   
  TOKEN_MINUS,  
  TOKEN_STAR,   
  TOKEN_SLASH,  
  TOKEN_PERCENT,

  /* increment / decrement */
  TOKEN_PLUSPLUS,  
  TOKEN_MINUSMINUS,

  /* punctuation */
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_SEMICOLON,
  TOKEN_COMMA,
  TOKEN_DOT,

  /* relational / equality */
  TOKEN_LT,
  TOKEN_GT,
  TOKEN_EQEQ,
  TOKEN_LTEQ,
  TOKEN_GTEQ,  
  TOKEN_BANGEQ,

  /* bitwise */
  TOKEN_AMP,   
  TOKEN_PIPE,  
  TOKEN_CARET, 
  TOKEN_TILDE, 
  TOKEN_LSHIFT,
  TOKEN_RSHIFT,

  /* logical */
  TOKEN_AMPAMP,  
  TOKEN_PIPEPIPE,
  TOKEN_BANG,
   
  /* misc */
  TOKEN_ARROW,
  
} Token_kind;

typedef struct {
  Token_kind kind;
  Keyword_kind kw_kind;
  const char *text;
  size_t text_len;
  unsigned long hash;
  size_t line;
  size_t col;
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
