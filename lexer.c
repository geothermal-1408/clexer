#include <ctype.h>
#include <string.h>

#include "lexer.h"

#define KEYWORD_TABLE_SIZE 64

// single character
#define SINGLE(k)       \
  do                    \
  {                     \
    token.kind = (k);   \
    token.text_len = 1; \
    advance(l);         \
    return token;       \
  } while (0)

// two character
#define DOUBLE(k)       \
  do                    \
  {                     \
    token.kind = (k);   \
    token.text_len = 2; \
    advance(l);         \
    advance(l);         \
    return token;       \
  } while (0)

// Three character
#define TRIPLE(k)       \
  do                    \
  {                     \
    token.kind = (k);   \
    token.text_len = 3; \
    advance(l);         \
    advance(l);         \
    advance(l);         \
    return token;       \
  } while (0)

#define LEX_LITERAL(delim, tok_kind)		  	\
  do							\
    {						  	\
    advance(l);					  	\
    size_t start_cursor = l->cursor;		  	\
    token.kind = (tok_kind);			  	\
    while(peek(l) != (delim) && peek(l) != '\0') {  	\
      if (peek(l) == '\\') advance(l);		  	\
      advance(l);					\
    }						  	\
    token.text = &l->content[start_cursor];	  	\
    token.text_len = l->cursor - start_cursor;	  	\
    if(peek(l) == (delim)) advance(l);			\
    return token;					\
  } while(0)						\
     
typedef struct
{
  const char *text;
  size_t len;
  int kw_kind;
} KeywordEntry;

static KeywordEntry keyword_table[KEYWORD_TABLE_SIZE];
static int keyword_table_init = 0;

static unsigned long hash_string(const char *str, size_t len)
{
  unsigned long hash = 5381;
  for (size_t i = 0; i < len; ++i)
  {
    hash = ((hash << 5) + hash) + str[i];
  }
  return hash;
}

static void insert_keyword(const char *text, int kw_kind)
{
  size_t len = strlen(text);
  unsigned long hash = hash_string(text, len);
  size_t index = hash % KEYWORD_TABLE_SIZE;

  // using linear probing
  while (keyword_table[index].text != NULL)
  {
    index = (index + 1) % KEYWORD_TABLE_SIZE;
  }
  keyword_table[index].text = text;
  keyword_table[index].len = len;
  keyword_table[index].kw_kind = kw_kind;
}

static void init_keyword_table(void)
{
  if (keyword_table_init)
    return;

  insert_keyword("int", KW_INT);
  insert_keyword("void", KW_VOID);
  insert_keyword("char", KW_CHAR);
  insert_keyword("return", KW_RETURN);
  insert_keyword("if", KW_IF);
  insert_keyword("else", KW_ELSE);
  insert_keyword("while", KW_WHILE);

  keyword_table_init = 1;
}

static char peek(Lexer *l)
{
  if (l->cursor >= l->content_len)
  {
    return '\0';
  }
  return l->content[l->cursor];
}

static char peek_next(Lexer *l)
{
  if (l->cursor + 1 >= l->content_len)
  {
    return '\0';
  }
  return l->content[l->cursor + 1];
}

static void advance(Lexer *l)
{
  if (l->cursor < l->content_len)
  {
    if (l->content[l->cursor] == '\n')
    {
      l->line++;
      l->bol = l->cursor + 1;
    }
    l->cursor++;
  }
}

static void skip_whitespace_commments(Lexer *l)
{
  while (l->cursor < l->content_len)
  {
    char c = peek(l);

    if (isspace((unsigned char)c))
    {
      advance(l);
      continue;
    }

    if (c == '/' && l->cursor + 1 < l->content_len)
    {
      char next_c = l->content[l->cursor + 1];

      if (next_c == '/')
      {
        while (l->cursor < l->content_len && peek(l) != '\n')
        {
          advance(l);
        }

        continue;
      }

      if (next_c == '*')
      {
        advance(l);
        advance(l);

        while (l->cursor < l->content_len)
        {
          if (peek(l) == '*' && l->cursor + 1 < l->content_len && l->content[l->cursor + 1] == '/')
          {
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

  while (keyword_table[index].text != NULL)
  {
    if (keyword_table[index].len == token->text_len &&
        strncmp(keyword_table[index].text, token->text, token->text_len) == 0)
    {
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

Token lexer_next(Lexer *l)
{
  skip_whitespace_commments(l);
  Token token = {0};
  token.text = &l->content[l->cursor];
  token.line = l->line;
  token.col = l->cursor - l->bol + 1;

  if (l->cursor >= l->content_len)
  {
    token.kind = TOKEN_END;
    token.text_len = 0;
    return token;
  }

  char c = peek(l);
  char next = peek_next(l);

  switch (c)
  {

  case '#':
    SINGLE(TOKEN_HASH);
  case '(':
    SINGLE(TOKEN_LPAREN);
  case ')':
    SINGLE(TOKEN_RPAREN);
  case '{':
    SINGLE(TOKEN_LBRACE);
  case '}':
    SINGLE(TOKEN_RBRACE);
  case ';':
    SINGLE(TOKEN_SEMICOLON);
  case ',':
    SINGLE(TOKEN_COMMA);
  case '.':
    SINGLE(TOKEN_DOT);
  case '~':
    SINGLE(TOKEN_TILDE);

  case '=':
    if (next == '=')
    {
      DOUBLE(TOKEN_EQEQ);
    }
    SINGLE(TOKEN_EQUALS);

  case '!':
    if (next == '=')
    {
      DOUBLE(TOKEN_BANGEQ);
    }
    SINGLE(TOKEN_BANG);

  case '+':
    if (next == '+')
    {
      DOUBLE(TOKEN_PLUSPLUS);
    }
    if (next == '=')
    {
      DOUBLE(TOKEN_PLUSEQ);
    }
    SINGLE(TOKEN_PLUS);

  case '-':
    if (next == '-')
    {
      DOUBLE(TOKEN_MINUSMINUS);
    }
    if (next == '=')
    {
      DOUBLE(TOKEN_MINUSEQ);
    }
    if (next == '>')
    {
      DOUBLE(TOKEN_ARROW);
    }
    SINGLE(TOKEN_MINUS);

  case '*':
    if (next == '=')
    {
      DOUBLE(TOKEN_STAREQ);
    }
    SINGLE(TOKEN_STAR);

  case '/':
    if (next == '=')
    {
      DOUBLE(TOKEN_SLASHEQ);
    }
    SINGLE(TOKEN_SLASH);

  case '%':
    if (next == '=')
    {
      DOUBLE(TOKEN_PERCENTEQ);
    }
    SINGLE(TOKEN_PERCENT);

  case '&':
    if (next == '&')
    {
      DOUBLE(TOKEN_AMPAMP);
    }
    if (next == '=')
    {
      DOUBLE(TOKEN_AMPEQ);
    }
    SINGLE(TOKEN_AMP);

  case '|':
    if (next == '|')
    {
      DOUBLE(TOKEN_PIPEPIPE);
    }
    if (next == '=')
    {
      DOUBLE(TOKEN_PIPEEQ);
    }
    SINGLE(TOKEN_PIPE);

  case '^':
    if (next == '=')
    {
      DOUBLE(TOKEN_CARETEQ);
    }
    SINGLE(TOKEN_CARET);

  case '<':
    if (next == '<')
    {
      /* peek two ahead for '=' */
      if (l->cursor + 2 < l->content_len &&
          l->content[l->cursor + 2] == '=')
      {
        TRIPLE(TOKEN_LSHIFTEQ);
      }
      DOUBLE(TOKEN_LSHIFT);
    }
    if (next == '=')
    {
      DOUBLE(TOKEN_LTEQ);
    }
    SINGLE(TOKEN_LT);

  case '>':
    if (next == '>')
    {
      if (l->cursor + 2 < l->content_len &&
          l->content[l->cursor + 2] == '=')
      {
        TRIPLE(TOKEN_RSHIFTEQ);
      }
      DOUBLE(TOKEN_RSHIFT);
    }
    if (next == '=')
    {
      DOUBLE(TOKEN_GTEQ);
    }
    SINGLE(TOKEN_GT);
  }

#undef SINGLE
#undef DOUBLE
#undef TRIPLE

  if (isdigit((unsigned char)c))
  {
    size_t start_cursor = l->cursor;
    token.kind = TOKEN_NUMBER;

    while (isdigit((unsigned char)peek(l)))
    {
      advance(l);
    }
    token.text_len = l->cursor - start_cursor;
    return token;
  }

  if (c == '"') {
    LEX_LITERAL('"', TOKEN_STRING);
  }

  if(c == '\'') {
    LEX_LITERAL('\'', TOKEN_CHAR);
  }
  
  if (isalpha((unsigned char)c) || c == '_')
  {
    size_t start_cursor = l->cursor;
    token.kind = TOKEN_SYMBOL;

    while (isalnum((unsigned char)peek(l)) || peek(l) == '_')
    {
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

#undef LEX_LITERAL

// this looks disgusting maybe use table/KV pair to manage this.

const char *token_kind_str(Token_kind kind)
{
    switch (kind) {
        case TOKEN_END:         return "EOF";
        case TOKEN_INVALID:     return "INVALID";
        case TOKEN_HASH:        return "'#'";
        case TOKEN_SYMBOL:      return "identifier";
        case TOKEN_KEYWORD:     return "keyword";
        case TOKEN_NUMBER:      return "number";
        case TOKEN_STRING:      return "string";
        case TOKEN_CHAR:        return "character";
        case TOKEN_LPAREN:      return "'('";
        case TOKEN_RPAREN:      return "')'";
        case TOKEN_LBRACE:      return "'{'";
        case TOKEN_RBRACE:      return "'}'";
        case TOKEN_SEMICOLON:   return "';'";
        case TOKEN_COMMA:       return "','";
        case TOKEN_DOT:         return "'.'";
        case TOKEN_LT:          return "'<'";
        case TOKEN_GT:          return "'>'";
        case TOKEN_LTEQ:        return "'<='";
        case TOKEN_GTEQ:        return "'>='";
        case TOKEN_EQEQ:        return "'=='";
        case TOKEN_BANGEQ:      return "'!='";
        case TOKEN_EQUALS:      return "'='";
        case TOKEN_PLUSEQ:      return "'+='";
        case TOKEN_MINUSEQ:     return "'-='";
        case TOKEN_STAREQ:      return "'*='";
        case TOKEN_SLASHEQ:     return "'/='";
        case TOKEN_PERCENTEQ:   return "'%='";
        case TOKEN_AMPEQ:       return "'&='";
        case TOKEN_PIPEEQ:      return "'|='";
        case TOKEN_CARETEQ:     return "'^='";
        case TOKEN_LSHIFTEQ:    return "'<<='";
        case TOKEN_RSHIFTEQ:    return "'>>='";
        case TOKEN_PLUS:        return "'+'";
        case TOKEN_MINUS:       return "'-'";
        case TOKEN_STAR:        return "'*'";
        case TOKEN_SLASH:       return "'/'";
        case TOKEN_PERCENT:     return "'%'";
        case TOKEN_PLUSPLUS:    return "'++'";
        case TOKEN_MINUSMINUS:  return "'--'";
        case TOKEN_AMP:         return "'&'";
        case TOKEN_PIPE:        return "'|'";
        case TOKEN_CARET:       return "'^'";
        case TOKEN_TILDE:       return "'~'";
        case TOKEN_LSHIFT:      return "'<<'";
        case TOKEN_RSHIFT:      return "'>>'";
        case TOKEN_AMPAMP:      return "'&&'";
        case TOKEN_PIPEPIPE:    return "'||'";
        case TOKEN_BANG:        return "'!'";
        case TOKEN_ARROW:       return "'->'";
    }
    return "unknown";
}

