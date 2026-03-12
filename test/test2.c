/* Block comment .
   What happens if we put // inside a block comment? 
*/

#include <stdio.h>

int main() {
    int a1 = 10;
    int _hidden_var = 20;
    
    //  equals vs double equals back-to-back
    if (a1 == _hidden_var) {
        a1 = a1 == 10; 
    }

    // currently lexer will parse this as NUMBER, DOT, NUMBER
    int fake_float = 3.14159;

    //  a string loaded with symbols that are usually single-character tokens
    printf("String test: # ( ) { } ; , < > . = ==");
    
    // empty string
    "";

    //  other keywords that aren't in the hash table yet
    while (a1 < 100) {
        break;
    }

    return 0;
}
