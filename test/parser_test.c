#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "ast.h"

int main(void)
{
    const char *src =

      //// STAGE 1 ////
      
      /* "int add(int a, int b) {\n" */
      /* "    int result = a + b;\n" */
      /* "    return result;\n" */
      /* "}\n"; */
   
      ////////////

      //// STAGE 2 ////
      
      /* "int f() {\n" */
      /* "    return 2 + 3 * 4;\n"  /\* must parse as 2 + (3 * 4) *\/ */
      /* "}\n"; */
      /* "int a;\n" */
      /* "int b;\n" */
      /* "int c;\n" */
      //"return a + b + c;"     /* left-assoc: should be (a+b)+c, not a+(b+c) */
      //"return a && b || c;"   /* || lower than &&: should be (a&&b)||c */
      //"return a < b == c < d;"     /* == lower than < */
      /* "}\n"; */
    
      ////////////

      //// STAGE 3 ////
      
      //     "int f() { int x = 1 }"; /* missing semicolon */

      /* "int f(int a { return a; }"; /\* missing closing paren *\/ */
      
      "int f() {}";  /* empty body */

      ////////////
    
    Parser  p    = parser_new(src, strlen(src));
    AstNode *ast = parser_parse(&p);

    if (!ast) {
        fprintf(stderr, "parse failed\n");
        return 1;
    }

    ast_print(ast, 0);
    ast_free(ast);
    return 0;
}
