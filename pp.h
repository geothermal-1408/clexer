#ifndef PP_H
#define PP_H

#include <stddef.h>
#include "lexer.h"

#define MAX_INCLUDE_DEPTH 32
#define MAX_MACROS 256

typedef struct {
  Lexer lexer;
  char *buf;
  char path[256];
} InputStream;

typedef struct {
  Token *tokens;
  size_t count;
  size_t capacity;
} TokenStream;

typedef struct {
  char name[64];
  Token replacement;
} MacroDef;

typedef struct {
  InputStream stack[MAX_INCLUDE_DEPTH];
  int top;
  MacroDef macros[MAX_MACROS];
  size_t macro_count;
  int had_error;
} Preprocessor;

void token_stream_free(TokenStream *ts);
Preprocessor pp_new(const char *src, size_t src_len, const char *path);
TokenStream pp_run(Preprocessor *p);
char *pp_read_file(const char *path, size_t *out_len);

#endif //PP_H
