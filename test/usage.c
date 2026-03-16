#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "lexer.h"
#include "common.h"
       
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

const char *print_kw_kind(Keyword_kind kw_kind)
{
  switch(kw_kind) {
  case KW_NONE:   return "KW_NONE";
  case KW_INT:    return "KW_INT";
  case KW_RETURN: return "KW_RETURN";
  case KW_IF:     return "KW_IF";
  default:        return "UNKNOWN_KW";
  }
  
}

void normal_test(Lexer *l, const char *source)
{
  printf("Lexing: '%s'\n",source);
  printf("--------------------\n");

  Token token;
  
  do {
    token = lexer_next(l);
    
    if(token.kind == TOKEN_KEYWORD) {
          printf("Line: %2zu | Kind: %-14s | HASH: %10lu | Text: '%.*s'\n",
		 l->line,
		 print_kw_kind(token.kw_kind),
		 token.hash,
		 (int)token.text_len,
		 token.text);

    } else {
    printf("Line: %2zu | Kind: %d | HASH: %10lu | Text: '%.*s'\n",
	   l->line,
	   token.kind,
	   token.hash,
	   (int)token.text_len,
	   token.text);
    }
  } while(token.kind != TOKEN_END && token.kind != TOKEN_INVALID);

}

void benchmark_test(const char *content, size_t len)
{
  int factor = 10000;
  size_t fat_len = len * factor;

  char *fat_content = malloc(fat_len + 1);

  assert(fat_content!=NULL);

  for(int i = 0; i < factor; ++i) {
    memcpy(fat_content + (i * len), content, len);
  }

  fat_content[fat_len] = '\0';

  printf("Benchmarking lexer on %zu bytes...\n",fat_len);

  clock_t start = clock();

  Lexer l = lexer_new(fat_content, fat_len);
  Token token;
  size_t token_count = 0;

  do{
    token = lexer_next(&l);
    token_count++;
    
  } while(token.kind != TOKEN_END && token.kind != TOKEN_INVALID);

  clock_t end = clock();
  double consumed_time = (double)(end - start)/CLOCKS_PER_SEC;

  printf("\n--- RESULTS ---\n");
  printf("Time taken:   %f seconds\n", consumed_time);
  printf("Tokens lexed: %zu\n", token_count);
    
  if (consumed_time > 0) {
    printf("Speed:        %.2f million tokens/sec\n", (token_count / consumed_time) / 1000000.0);
    printf("Throughput:   %.2f MB/sec\n", (fat_len / consumed_time) / (1024 * 1024));
  }

  free(fat_content);
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    printf("Usage: %s <file_to_benchmark>\n", argv[0]);
    return 1;
  }

  bool is_bench = false;
  const char *source = NULL;

  if(strcmp(argv[1], "--bench") == 0) {
    is_bench = true;
    if(argc < 3) {
      printf("Error: Missing filename after --bench flag.\n");
      return 1;
    }

    source = argv[2];
  } else {
    source = argv[1];
  }
  
  size_t len = 0;
  char *content = read_file(source, &len);
  if(!content) return 1;

  if(is_bench) {
    benchmark_test(content, len);
  } else {
    Lexer l = lexer_new(content, len);
    normal_test(&l, source);
  }
  
  free(content);
  return 0;
}
