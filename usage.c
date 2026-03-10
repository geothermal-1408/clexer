#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

char *read_file(const char *file, size_t *out_len)
{
  FILE *fp;
  fp = fopen(file, "rb");
  assert(fp != NULL);

  fseek(fp, 0, SEEK_END);
  long length = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char *buffer = malloc(length + 1);
  assert(buffer != NULL);

  size_t read_len = fread(buffer, 1, length, fp);
  buffer[read_len] = '\0';
  fclose(fp);

  if(out_len) *out_len = read_len;
  return buffer;
}

int main(int argc, char **argv)
{
  const char *source = argv[1];
  size_t len = 0;

  char *content = read_file(source, &len);
  if(!content) return 1;
 
  printf("Lexing: '%s'\n",source);
  printf("--------------------\n");

  Lexer l = lexer_new(content, len);
  Token token;
  
  do {
    token = lexer_next(&l);

    printf("Line: %2zu | Kind: %d | HASH: %10lu | Text: '%.*s'\n",
	   l.line,
	   token.kind,
	   token.hash,
	   (int)token.text_len,
	   token.text);
  } while(token.kind != TOKEN_END && token.kind != TOKEN_INVALID);

  free(content);
  return 0;
}
