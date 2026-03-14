#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"

/* ── test infrastructure ─────────────────────────────────────────────────── */

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(desc, expr)                                               \
    do {                                                                \
        tests_run++;                                                    \
        if (expr) {                                                     \
            tests_passed++;                                             \
            printf("  PASS  %s\n", desc);                              \
        } else {                                                        \
            tests_failed++;                                             \
            printf("  FAIL  %s  (%s:%d)\n", desc, __FILE__, __LINE__); \
        }                                                               \
    } while (0)

/* Lex the entire input and return the Nth token (0-indexed). */
static Token get_token(const char *src, int n)
{
    Lexer l = lexer_new(src, strlen(src));
    Token t = {0};
    for (int i = 0; i <= n; i++)
        t = lexer_next(&l);
    return t;
}

/* Convenience: lex src and collect every token until TOKEN_END. */
#define MAX_TOKENS 128
static int lex_all(const char *src, Token *out)
{
    Lexer l = lexer_new(src, strlen(src));
    int   n = 0;
    while (n < MAX_TOKENS) {
        out[n] = lexer_next(&l);
        if (out[n].kind == TOKEN_END) break;
        n++;
    }
    return n; /* count excludes TOKEN_END */
}

/* ── helpers ─────────────────────────────────────────────────────────────── */

static int tok_text_eq(Token *t, const char *expected)
{
    size_t elen = strlen(expected);
    return t->text_len == elen && strncmp(t->text, expected, elen) == 0;
}

/* ── test groups ─────────────────────────────────────────────────────────── */

static void test_empty_input(void)
{
    printf("\n[empty / whitespace input]\n");

    Lexer l = lexer_new("", 0);
    Token t = lexer_next(&l);
    CHECK("empty string -> TOKEN_END", t.kind == TOKEN_END);

    l = lexer_new("   \t\n  ", 7);
    t = lexer_next(&l);
    CHECK("whitespace only -> TOKEN_END", t.kind == TOKEN_END);
}

static void test_keywords(void)
{
    printf("\n[keywords]\n");

    Token t;
    t = get_token("int", 0);
    CHECK("int -> TOKEN_KEYWORD",  t.kind    == TOKEN_KEYWORD);
    CHECK("int -> KW_INT",         t.kw_kind == KW_INT);

    t = get_token("return", 0);
    CHECK("return -> TOKEN_KEYWORD", t.kind    == TOKEN_KEYWORD);
    CHECK("return -> KW_RETURN",     t.kw_kind == KW_RETURN);

    t = get_token("if", 0);
    CHECK("if -> TOKEN_KEYWORD", t.kind    == TOKEN_KEYWORD);
    CHECK("if -> KW_IF",         t.kw_kind == KW_IF);

    /* keyword prefix must not match */
    t = get_token("integer", 0);
    CHECK("integer -> TOKEN_SYMBOL (not keyword)", t.kind == TOKEN_SYMBOL);

    t = get_token("iffy", 0);
    CHECK("iffy -> TOKEN_SYMBOL (not keyword)", t.kind == TOKEN_SYMBOL);
}

static void test_identifiers(void)
{
    printf("\n[identifiers]\n");

    Token t = get_token("foo", 0);
    CHECK("simple ident kind",  t.kind == TOKEN_SYMBOL);
    CHECK("simple ident text",  tok_text_eq(&t, "foo"));

    t = get_token("_bar123", 0);
    CHECK("underscore-led ident", t.kind == TOKEN_SYMBOL && tok_text_eq(&t, "_bar123"));

    t = get_token("__FILE__", 0);
    CHECK("double-underscore ident", t.kind == TOKEN_SYMBOL);

    /* identifier followed immediately by punctuation */
    Token out[MAX_TOKENS];
    int n = lex_all("foo(", out);
    CHECK("ident then lparen: 2 tokens",        n == 2);
    CHECK("ident then lparen: first is SYMBOL",  out[0].kind == TOKEN_SYMBOL);
    CHECK("ident then lparen: second is LPAREN", out[1].kind == TOKEN_LPAREN);
}

static void test_numbers(void)
{
    printf("\n[numbers]\n");

    Token t = get_token("0", 0);
    CHECK("zero",                t.kind == TOKEN_NUMBER && tok_text_eq(&t, "0"));

    t = get_token("42", 0);
    CHECK("multi-digit",         t.kind == TOKEN_NUMBER && tok_text_eq(&t, "42"));

    t = get_token("1234567890", 0);
    CHECK("long number",         t.kind == TOKEN_NUMBER && tok_text_eq(&t, "1234567890"));

    /* number followed by semicolon */
    Token out[MAX_TOKENS];
    int n = lex_all("99;", out);
    CHECK("number then semicolon: 2 tokens",          n == 2);
    CHECK("number then semicolon: first is NUMBER",   out[0].kind == TOKEN_NUMBER);
    CHECK("number then semicolon: second is SEMICOLON", out[1].kind == TOKEN_SEMICOLON);
}

static void test_strings(void)
{
    printf("\n[string literals]\n");

    Token t = get_token("\"hello\"", 0);
    CHECK("simple string kind",  t.kind == TOKEN_STRING);
    CHECK("simple string text",  tok_text_eq(&t, "hello"));

    t = get_token("\"\"", 0);
    CHECK("empty string kind",   t.kind == TOKEN_STRING);
    CHECK("empty string len",    t.text_len == 0);

    t = get_token("\"hello\\nworld\"", 0);
    CHECK("escape sequence in string: kind", t.kind == TOKEN_STRING);
    /* text_len includes everything between the quotes */
    CHECK("escape sequence in string: non-zero len", t.text_len > 0);

    t = get_token("\"with \\\"quotes\\\" inside\"", 0);
    CHECK("escaped inner quotes: kind", t.kind == TOKEN_STRING);
}

static void test_comments(void)
{
    printf("\n[comments]\n");

    Token out[MAX_TOKENS];

    int n = lex_all("// full line comment\nfoo", out);
    CHECK("line comment skipped, ident follows", n == 1 && out[0].kind == TOKEN_SYMBOL);

    n = lex_all("/* block */ bar", out);
    CHECK("block comment skipped, ident follows", n == 1 && out[0].kind == TOKEN_SYMBOL);

    n = lex_all("a /* mid */ b", out);
    CHECK("mid-line block comment: 2 tokens",          n == 2);
    CHECK("mid-line block comment: first is 'a'",      tok_text_eq(&out[0], "a"));
    CHECK("mid-line block comment: second is 'b'",     tok_text_eq(&out[1], "b"));

    n = lex_all("/* unterminated", out);
    CHECK("unterminated block comment -> TOKEN_END", n == 0);
}

static void test_single_char_operators(void)
{
    printf("\n[single-character operators]\n");

    struct { const char *src; Token_kind kind; } cases[] = {
        { "+",  TOKEN_PLUS      },
        { "-",  TOKEN_MINUS     },
        { "*",  TOKEN_STAR      },
        { "/",  TOKEN_SLASH     },
        { "%",  TOKEN_PERCENT   },
        { "&",  TOKEN_AMP       },
        { "|",  TOKEN_PIPE      },
        { "^",  TOKEN_CARET     },
        { "~",  TOKEN_TILDE     },
        { "!",  TOKEN_BANG      },
        { "<",  TOKEN_LT        },
        { ">",  TOKEN_GT        },
        { "=",  TOKEN_EQUALS    },
        { "(",  TOKEN_LPAREN    },
        { ")",  TOKEN_RPAREN    },
        { "{",  TOKEN_LBRACE    },
        { "}",  TOKEN_RBRACE    },
        { ";",  TOKEN_SEMICOLON },
        { ",",  TOKEN_COMMA     },
        { ".",  TOKEN_DOT       },
        { "#",  TOKEN_HASH      },
    };

    char desc[64];
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        Token t = get_token(cases[i].src, 0);
        snprintf(desc, sizeof desc, "'%s' -> correct kind", cases[i].src);
        CHECK(desc, t.kind == cases[i].kind);
        CHECK("text_len == 1", t.text_len == 1);
    }
}

static void test_double_char_operators(void)
{
    printf("\n[two-character operators]\n");

    struct { const char *src; Token_kind kind; } cases[] = {
        { "==", TOKEN_EQEQ      },
        { "!=", TOKEN_BANGEQ    },
        { "<=", TOKEN_LTEQ      },
        { ">=", TOKEN_GTEQ      },
        { "++", TOKEN_PLUSPLUS  },
        { "--", TOKEN_MINUSMINUS},
        { "+=", TOKEN_PLUSEQ    },
        { "-=", TOKEN_MINUSEQ   },
        { "*=", TOKEN_STAREQ    },
        { "/=", TOKEN_SLASHEQ   },
        { "%=", TOKEN_PERCENTEQ },
        { "&=", TOKEN_AMPEQ     },
        { "|=", TOKEN_PIPEEQ    },
        { "^=", TOKEN_CARETEQ   },
        { "&&", TOKEN_AMPAMP    },
        { "||", TOKEN_PIPEPIPE  },
        { "<<", TOKEN_LSHIFT    },
        { ">>", TOKEN_RSHIFT    },
        { "->", TOKEN_ARROW     },
    };

    char desc[64];
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        Token t = get_token(cases[i].src, 0);
        snprintf(desc, sizeof desc, "'%s' -> correct kind", cases[i].src);
        CHECK(desc, t.kind == cases[i].kind);
        CHECK("text_len == 2", t.text_len == 2);
    }
}

static void test_triple_char_operators(void)
{
    printf("\n[three-character operators]\n");

    Token t = get_token("<<=", 0);
    CHECK("'<<=' -> TOKEN_LSHIFTEQ", t.kind == TOKEN_LSHIFTEQ);
    CHECK("'<<=' text_len == 3",     t.text_len == 3);

    t = get_token(">>=", 0);
    CHECK("'>>=' -> TOKEN_RSHIFTEQ", t.kind == TOKEN_RSHIFTEQ);
    CHECK("'>>=' text_len == 3",     t.text_len == 3);
}

static void test_maximal_munch(void)
{
    printf("\n[maximal munch — no off-by-one mislex]\n");

    Token out[MAX_TOKENS];
    int   n;

    /* "<<=" must not be lexed as TOKEN_LSHIFT + TOKEN_EQUALS */
    n = lex_all("<<=", out);
    CHECK("'<<=' is ONE token",          n == 1);
    CHECK("'<<=' kind is LSHIFTEQ",      out[0].kind == TOKEN_LSHIFTEQ);

    /* ">>" must not be lexed as two ">" */
    n = lex_all(">>", out);
    CHECK("'>>' is ONE token",           n == 1);
    CHECK("'>>' kind is RSHIFT",         out[0].kind == TOKEN_RSHIFT);

    /* "++" must not split into two "+" */
    n = lex_all("++", out);
    CHECK("'++' is ONE token",           n == 1);
    CHECK("'++' kind is PLUSPLUS",       out[0].kind == TOKEN_PLUSPLUS);

    /* "&&" must not split into two "&" */
    n = lex_all("&&", out);
    CHECK("'&&' is ONE token",           n == 1);
    CHECK("'&&' kind is AMPAMP",         out[0].kind == TOKEN_AMPAMP);

    /* "->" must not split into "-" and ">" */
    n = lex_all("->", out);
    CHECK("'->' is ONE token",           n == 1);
    CHECK("'->' kind is ARROW",          out[0].kind == TOKEN_ARROW);

    /* "!=" must not be "!" then "=" */
    n = lex_all("!=", out);
    CHECK("'!=' is ONE token",           n == 1);
    CHECK("'!=' kind is BANGEQ",         out[0].kind == TOKEN_BANGEQ);
}

static void test_operator_boundaries(void)
{
    printf("\n[operator boundary / adjacent token separation]\n");

    Token out[MAX_TOKENS];
    int   n;

    /* "a+b" — no spaces */
    n = lex_all("a+b", out);
    CHECK("a+b: 3 tokens",        n == 3);
    CHECK("a+b: [0] SYMBOL",      out[0].kind == TOKEN_SYMBOL);
    CHECK("a+b: [1] PLUS",        out[1].kind == TOKEN_PLUS);
    CHECK("a+b: [2] SYMBOL",      out[2].kind == TOKEN_SYMBOL);

    /* "a++b" — ++ then b */
    n = lex_all("a++b", out);
    CHECK("a++b: 3 tokens",       n == 3);
    CHECK("a++b: [1] PLUSPLUS",   out[1].kind == TOKEN_PLUSPLUS);

    /* "a---b" — a, --, -, b */
    n = lex_all("a---b", out);
    CHECK("a---b: 4 tokens",      n == 4);
    CHECK("a---b: [1] MINUSMINUS",out[1].kind == TOKEN_MINUSMINUS);
    CHECK("a---b: [2] MINUS",     out[2].kind == TOKEN_MINUS);

    /* "x<<=1" */
    n = lex_all("x<<=1", out);
    CHECK("x<<=1: 3 tokens",      n == 3);
    CHECK("x<<=1: [1] LSHIFTEQ",  out[1].kind == TOKEN_LSHIFTEQ);
    CHECK("x<<=1: [2] NUMBER",    out[2].kind == TOKEN_NUMBER);
}

static void test_line_col_tracking(void)
{
    printf("\n[line / column tracking]\n");

    Token out[MAX_TOKENS];
    lex_all("a\nb\nc", out);
    CHECK("'a' on line 1",  out[0].line == 1);
    CHECK("'a' at col 1",   out[0].col  == 1);
    CHECK("'b' on line 2",  out[1].line == 2);
    CHECK("'b' at col 1",   out[1].col  == 1);
    CHECK("'c' on line 3",  out[2].line == 3);

    lex_all("foo bar", out);
    CHECK("'foo' col 1",    out[0].col == 1);
    CHECK("'bar' col 5",    out[1].col == 5);
}

static void test_invalid_tokens(void)
{
    printf("\n[invalid / unrecognised tokens]\n");

    Token t = get_token("@", 0);
    CHECK("'@' -> TOKEN_INVALID",  t.kind == TOKEN_INVALID);
    CHECK("'@' text_len == 1",     t.text_len == 1);

    t = get_token("`", 0);
    CHECK("'`' -> TOKEN_INVALID",  t.kind == TOKEN_INVALID);

    /* lexer must recover: invalid then valid */
    Token out[MAX_TOKENS];
    int n = lex_all("@ foo", out);
    CHECK("invalid then ident: 2 tokens",       n == 2);
    CHECK("invalid then ident: [0] INVALID",    out[0].kind == TOKEN_INVALID);
    CHECK("invalid then ident: [1] SYMBOL",     out[1].kind == TOKEN_SYMBOL);
}

static void test_real_c_snippet(void)
{
    printf("\n[realistic C snippet]\n");

    const char *src =
        "int add(int a, int b) {\n"
        "    return a + b;\n"
        "}\n";

    Token out[MAX_TOKENS];
    int   n = lex_all(src, out);

    /* int add ( int a , int b ) { return a + b ; } */
    CHECK("snippet: 16 tokens", n == 16);
    CHECK("snippet: [0]  KW_INT",     out[0].kind == TOKEN_KEYWORD && out[0].kw_kind == KW_INT);
    CHECK("snippet: [1]  'add'",      out[1].kind == TOKEN_SYMBOL  && tok_text_eq(&out[1], "add"));
    CHECK("snippet: [2]  LPAREN",     out[2].kind == TOKEN_LPAREN);
    CHECK("snippet: [3]  KW_INT",     out[3].kind == TOKEN_KEYWORD);
    CHECK("snippet: [4]  'a'",        out[4].kind == TOKEN_SYMBOL  && tok_text_eq(&out[4], "a"));
    CHECK("snippet: [5]  COMMA",      out[5].kind == TOKEN_COMMA);
    CHECK("snippet: [6]  KW_INT",     out[6].kind == TOKEN_KEYWORD);
    CHECK("snippet: [7]  'b'",        out[7].kind == TOKEN_SYMBOL  && tok_text_eq(&out[7], "b"));
    CHECK("snippet: [8]  RPAREN",     out[8].kind == TOKEN_RPAREN);
    CHECK("snippet: [9]  LBRACE",     out[9].kind == TOKEN_LBRACE);
    CHECK("snippet: [10] KW_RETURN",  out[10].kind == TOKEN_KEYWORD && out[10].kw_kind == KW_RETURN);
    CHECK("snippet: [11] 'a'",        out[11].kind == TOKEN_SYMBOL);
    CHECK("snippet: [12] PLUS",       out[12].kind == TOKEN_PLUS);
    CHECK("snippet: [13] 'b'",        out[13].kind == TOKEN_SYMBOL);
    CHECK("snippet: [14] SEMICOLON",  out[14].kind == TOKEN_SEMICOLON);
    CHECK("snippet: [15] RBRACE",     out[15].kind == TOKEN_RBRACE);
}

/* ── entry point ─────────────────────────────────────────────────────────── */

int main(void)
{
    printf("=== lexer test suite ===");

    test_empty_input();
    test_keywords();
    test_identifiers();
    test_numbers();
    test_strings();
    test_comments();
    test_single_char_operators();
    test_double_char_operators();
    test_triple_char_operators();
    test_maximal_munch();
    test_operator_boundaries();
    test_line_col_tracking();
    test_invalid_tokens();
    test_real_c_snippet();

    printf("\n========================\n");
    printf("run: %d  pass: %d  fail: %d\n", tests_run, tests_passed, tests_failed);
    printf("========================\n");

    return tests_failed > 0 ? 1 : 0;
}
