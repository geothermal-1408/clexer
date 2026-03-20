#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pp.h"

char *pp_read_file(const char *path, size_t *out_len)
{
  printf("opening file %s\n",path);
  FILE *f= fopen(path, "r");
  if(!f) {
    fprintf(stderr, "preprocessor: can't open '%s'\n", path);
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  size_t size = (size_t)ftell(f);
  rewind(f);

  char *buf = malloc(size + 1);
  if(!buf) {
    fclose(f);
    return NULL;
  }
  fread(buf, 1, size, f);
  buf[size] = '\0';
  fclose(f);
  *out_len = size;
  return buf;
}

/* Token Stream */
static void ts_push(TokenStream *ts, Token t)
{
  if(ts->count >= ts->capacity) {
    size_t cap = ts->capacity == 0 ? 256 : ts->capacity * 2;
    ts->tokens = realloc(ts->tokens, cap * sizeof(Token));
    ts->capacity = cap;
  }

  ts->tokens[ts->count++] = t;
}

void token_stream_free(TokenStream *ts)
{
  free(ts->tokens);
  ts->tokens   = NULL;
  ts->count    = 0;
  ts->capacity = 0;
}

/* Input stream stack */

static Token next_raw_token(Preprocessor *pp)
{
  for(;;) {
    if(pp->top < 0) {
      return (Token){.kind = TOKEN_END};
    }
    Token t = lexer_next(&pp->stack[pp->top].lexer);

    if(t.kind == TOKEN_END) {
      free(pp->stack[pp->top].buf);
      pp->top--;
      continue;
    }

    return t;
  }
}

static void push_file(Preprocessor *pp,
		      char *buf,
		      size_t len,
		      const char *path)
{
  if(pp->top + 1 >= MAX_INCLUDE_DEPTH) {
    fprintf(stderr, "preprocessor: depth limit reached\n");
    pp->had_error = 1;
    free(buf);
    return;
  }

  pp->top++;
  pp->stack[pp->top].lexer = lexer_new(buf, len);
  pp->stack[pp->top].buf = buf;

  strncpy(pp->stack[pp->top].path, path ? path : "<unknown>", 255);
  pp->stack[pp->top].path[255] = '\0';
}

/* macro table */
static void macro_define(Preprocessor *pp,
			 const char *name,
			 size_t name_len,
			 Token replacement)
{
  for(size_t i = 0; i < pp->macro_count; ++i){
    if(strncmp(pp->macros[i].name, name, name_len) == 0 &&
       pp->macros[i].name[name_len] == '\0') {
      pp->macros[i].replacement = replacement;
      return;
    }
  }

    if(pp->macro_count >= MAX_MACROS) {
      fprintf(stderr, "preprocessor: macro table full\n");
      pp->had_error = 1;
      return;
    }

    size_t copy = name_len < 63 ? name_len:63;
    strncpy(pp->macros[pp->macro_count].name, name,copy);
    pp->macros[pp->macro_count].name[copy] = '\0';
    pp->macros[pp->macro_count].replacement = replacement;
    pp->macro_count++;
}

static Token *macro_lookup(Preprocessor *pp, const char *name, size_t len)
{
  for(size_t i = 0; i < pp->macro_count; ++i) {
    if(strncmp(pp->macros[i].name, name, len) == 0 &&
       pp->macros[i].name[len] == '\0') return &pp->macros[i].replacement; 
  }
  return NULL;
}

/* consume tokens until end of line */
static void skip_to_col(Preprocessor *pp, size_t directive_line)
{
  for(;;) {
    if(pp->top < 0) return;
    
    Lexer saved = pp->stack[pp->top].lexer;
    Token t = lexer_next(&pp->stack[pp->top].lexer);
    
    if(t.kind == TOKEN_END || t.line != directive_line) {
      pp->stack[pp->top].lexer = saved;
      break;
    }
  }
}

static void handle_include(Preprocessor *pp, size_t dir_line)
{
  Token path_tok = next_raw_token(pp);

  if(path_tok.kind == TOKEN_STRING) {
    char path[256] = {0};
    size_t plen = path_tok.text_len < 255 ? path_tok.text_len : 255;
    strncpy(path, path_tok.text, plen);
    path[plen] = '\0';

    char resolved[512] = {0};
    const char *current_path = pp->stack[pp->top].path;
    const char *last_slash   = strrchr(current_path, '/');

    if (last_slash) {
      /* copy directory part: "src/test/" from "src/test/hello.c" */
      size_t dir_len = (size_t)(last_slash - current_path) + 1;
      strncpy(resolved, current_path, dir_len);
      strncpy(resolved + dir_len, path, 511 - dir_len);
    } else {
      /* current file has no directory component — use path as-is */
      strncpy(resolved, path, 511);
    }

    size_t file_len = 0;
    char *buf = pp_read_file(resolved, &file_len);
    if(!buf) {
      pp->had_error = 1;
      return;
    }
    
    push_file(pp, buf, file_len, path);
    fprintf(stderr, "[DEBUG]: macro included %s\n",resolved);
  } else {
    skip_to_col(pp, dir_line);
  }
}

static void handle_define(Preprocessor *pp, size_t dir_line)
{
  Token name = next_raw_token(pp);
  if(name.kind != TOKEN_SYMBOL) {
    fprintf(stderr, "preprocessor: expected macro name\n");
    pp->had_error = 1;
    skip_to_col(pp, dir_line);
    return;
  }

  Lexer saved_lexer = pp->stack[pp->top].lexer;
  Token value = next_raw_token(pp);

  if(value.kind == TOKEN_END || value.line != dir_line) {
    pp->stack[pp->top].lexer = saved_lexer;
    Token empty = {0};
    macro_define(pp, name.text, name.text_len, empty);
    return;
  }
  macro_define(pp, name.text, name.text_len, value);
  skip_to_col(pp, dir_line);
}

Preprocessor pp_new(const char *src, size_t src_len, const char *path)
{
  Preprocessor pp = {0};
  pp.top = 0;
  pp.stack[0].lexer = lexer_new(src, src_len);
  pp.stack[0].buf = NULL;
  strncpy(pp.stack[pp.top].path, path? path : "<root>", 255);
  return pp;
}

TokenStream pp_run(Preprocessor *pp)
{
  TokenStream ts = {0};

  for(;;) {
    Token t = next_raw_token(pp);
    if(t.kind == TOKEN_END) break;

    if(t.kind == TOKEN_HASH) {
      Token directive = next_raw_token(pp);
      if(directive.kind != TOKEN_SYMBOL) {
	skip_to_col(pp, t.line);
	continue;
      }
      if(strncmp(directive.text, "include", 7) == 0 &&
	 directive.text_len == 7) {
	handle_include(pp, t.line);
      }
      else if(strncmp(directive.text, "define", 6) == 0 &&
	      directive.text_len == 6) {
	handle_define(pp, t.line);
      }
      else {
	skip_to_col(pp, t.line);
      }
      continue;
    }

    if(t.kind == TOKEN_SYMBOL) {
      Token *rep = macro_lookup(pp, t.text, t.text_len);
      if(rep && rep->kind != TOKEN_END) {
	ts_push(&ts, *rep);
	continue;
      }
    }

    ts_push(&ts, t);
  }

  return ts;
}
