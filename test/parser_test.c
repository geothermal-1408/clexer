#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "ast.h"

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(desc, expr)                                                   \
    do {                                                                    \
        tests_run++;                                                        \
        if (expr) {                                                         \
            tests_passed++;                                                 \
            printf("    PASS  %s\n", desc);                                 \
        } else {                                                            \
            tests_failed++;                                                 \
            printf("    FAIL  %s  (%s:%d)\n", desc, __FILE__, __LINE__);   \
        }                                                                   \
    } while (0)

static AstNode *parse_and_check(const char *src, int print_tree)
{
    Parser  p   = parser_new(src, strlen(src));
    AstNode *ast = parser_parse(&p);

    if (print_tree) {
        if (ast) ast_print(ast, 0);
        else     fprintf(stderr, "  (parse failed — no tree)\n");
        printf("\n");
    }
    return ast;
}

static AstNode *func_body_stmt(AstNode *ast, size_t fn_idx, size_t stmt_idx)
{
    if (!ast || ast->program.count <= fn_idx) return NULL;
    AstNode *fn = ast->program.items[fn_idx];
    if (!fn || fn->kind != NODE_FUNC_DEF) return NULL;
    AstNode *body = fn->func_def.body;
    if (!body || body->block.count <= stmt_idx) return NULL;
    return body->block.stmts[stmt_idx];
}

static void test_func(void)
{
    printf("\n[func] — function definitions\n");

    /* no-param function */
    {
        const char *src = "int foo() { return 1; }";
        AstNode *ast = parse_and_check(src, 1);
        CHECK("no-param func: parse ok",          ast != NULL);
        CHECK("no-param func: 1 decl",            ast && ast->program.count == 1);
        AstNode *fn = ast ? ast->program.items[0] : NULL;
        CHECK("no-param func: is NODE_FUNC_DEF",  fn && fn->kind == NODE_FUNC_DEF);
        CHECK("no-param func: 0 params",          fn && fn->func_def.params.count == 0);
        ast_free(ast);
    }

    /* two-param function */
    {
        const char *src = "int add(int a, int b) { return a + b; }";
        AstNode *ast = parse_and_check(src, 1);
        AstNode *fn  = ast ? ast->program.items[0] : NULL;
        CHECK("two-param func: 2 params",         fn && fn->func_def.params.count == 2);
        AstNode *p0  = fn ? fn->func_def.params.items[0] : NULL;
        AstNode *p1  = fn ? fn->func_def.params.items[1] : NULL;
        CHECK("two-param func: param0 is NODE_PARAM", p0 && p0->kind == NODE_PARAM);
        CHECK("two-param func: param1 is NODE_PARAM", p1 && p1->kind == NODE_PARAM);
        ast_free(ast);
    }

    /* multiple top-level functions */
    {
        const char *src =
            "int f() { return 1; }\n"
            "int g() { return 2; }\n";
        AstNode *ast = parse_and_check(src, 1);
        CHECK("multi-func: 2 decls", ast && ast->program.count == 2);
        ast_free(ast);
    }
}

static void test_var(void)
{
    printf("\n[var] — variable declarations\n");

    /* declaration without initialiser */
    {
        const char *src = "int f() { int x; return x; }";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *stmt = func_body_stmt(ast, 0, 0);
        CHECK("var no-init: NODE_VAR_DECL",   stmt && stmt->kind == NODE_VAR_DECL);
        CHECK("var no-init: no init node",    stmt && stmt->var_decl.init == NULL);
        ast_free(ast);
    }

    /* declaration with initialiser */
    {
        const char *src = "int f() { int x = 42; return x; }";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *stmt = func_body_stmt(ast, 0, 0);
        CHECK("var with init: NODE_VAR_DECL", stmt && stmt->kind == NODE_VAR_DECL);
        CHECK("var with init: init is number",
              stmt && stmt->var_decl.init &&
              stmt->var_decl.init->kind == NODE_NUMBER);
        ast_free(ast);
    }

    /* declaration with expression initialiser */
    {
        const char *src = "int f() { int x = 1 + 2; return x; }";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *stmt = func_body_stmt(ast, 0, 0);
        CHECK("var expr-init: init is binary",
              stmt && stmt->var_decl.init &&
              stmt->var_decl.init->kind == NODE_BINARY);
        ast_free(ast);
    }
}

static void test_return(void)
{
    printf("\n[return] — return statements\n");

    /* return with value */
    {
        const char *src = "int f() { return 99; }";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *stmt = func_body_stmt(ast, 0, 0);
        CHECK("return value: NODE_RETURN",    stmt && stmt->kind == NODE_RETURN);
        CHECK("return value: value not null", stmt && stmt->ret.value != NULL);
        CHECK("return value: value is number",
              stmt && stmt->ret.value &&
              stmt->ret.value->kind == NODE_NUMBER);
        ast_free(ast);
    }

    /* bare return */
    {
        const char *src = "void f() { return; }";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *stmt = func_body_stmt(ast, 0, 0);
        CHECK("bare return: NODE_RETURN",    stmt && stmt->kind == NODE_RETURN);
        CHECK("bare return: value is NULL",  stmt && stmt->ret.value == NULL);
        ast_free(ast);
    }
}

static void test_assign(void)
{
    printf("\n[assign] — assignment statements\n");

    /* simple assignment */
    {
        const char *src = "int f() { int x = 0; x = 5; return x; }";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *stmt = func_body_stmt(ast, 0, 1);
        CHECK("assign: NODE_ASSIGN",       stmt && stmt->kind == NODE_ASSIGN);
        CHECK("assign: value not null",    stmt && stmt->assign.value != NULL);
        CHECK("assign: value is number",
              stmt && stmt->assign.value &&
              stmt->assign.value->kind == NODE_NUMBER);
        ast_free(ast);
    }

    /* assignment with expression */
    {
        const char *src = "int f() { int x = 0; x = x + 1; return x; }";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *stmt = func_body_stmt(ast, 0, 1);
        CHECK("assign expr: value is binary",
              stmt && stmt->assign.value &&
              stmt->assign.value->kind == NODE_BINARY);
        ast_free(ast);
    }
}

static void test_if(void)
{
    printf("\n[if] — if statements\n");

    /* plain if, no else */
    {
        const char *src = "int f(int x) { if (x) { x = 1; } return x; }";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *stmt = func_body_stmt(ast, 0, 0);
        CHECK("if: NODE_IF",              stmt && stmt->kind == NODE_IF);
        CHECK("if: condition not null",   stmt && stmt->if_stmt.condition != NULL);
        CHECK("if: then_block not null",  stmt && stmt->if_stmt.then_block != NULL);
        CHECK("if: else_block is NULL",   stmt && stmt->if_stmt.else_block == NULL);
        ast_free(ast);
    }

    /* if with else */
    {
        const char *src =
            "int f(int x) {\n"
            "    if (x) { x = 1; } else { x = 0; }\n"
            "    return x;\n"
            "}\n";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *stmt = func_body_stmt(ast, 0, 0);
        CHECK("if-else: NODE_IF",              stmt && stmt->kind == NODE_IF);
        CHECK("if-else: else_block not NULL",  stmt && stmt->if_stmt.else_block != NULL);
        CHECK("if-else: else is NODE_BLOCK",
              stmt && stmt->if_stmt.else_block &&
              stmt->if_stmt.else_block->kind == NODE_BLOCK);
        ast_free(ast);
    }

    /* else-if chain */
    {
        const char *src =
            "int f(int x) {\n"
            "    if (x) { x = 1; } else if (x) { x = 2; } else { x = 3; }\n"
            "    return x;\n"
            "}\n";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *stmt = func_body_stmt(ast, 0, 0);
        CHECK("else-if: outer is NODE_IF",      stmt && stmt->kind == NODE_IF);
        CHECK("else-if: else_block is NODE_IF",
              stmt && stmt->if_stmt.else_block &&
              stmt->if_stmt.else_block->kind == NODE_IF);
        AstNode *inner = stmt ? stmt->if_stmt.else_block : NULL;
        CHECK("else-if: inner else is NODE_BLOCK",
              inner && inner->if_stmt.else_block &&
              inner->if_stmt.else_block->kind == NODE_BLOCK);
        ast_free(ast);
    }

    /* if condition is comparison */
    {
        const char *src = "int f(int a, int b) { if (a == b) { a = 0; } return a; }";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *stmt = func_body_stmt(ast, 0, 0);
        CHECK("if-cmp: condition is binary",
              stmt && stmt->if_stmt.condition &&
              stmt->if_stmt.condition->kind == NODE_BINARY);
        CHECK("if-cmp: condition op is ==",
              stmt && stmt->if_stmt.condition &&
              stmt->if_stmt.condition->binary.op == TOKEN_EQEQ);
        ast_free(ast);
    }
}

static void test_while(void)
{
    printf("\n[while] — while loops\n");

    /* basic while */
    {
        const char *src =
            "int f() {\n"
            "    int i = 0;\n"
            "    while (i < 10) { i = i + 1; }\n"
            "    return i;\n"
            "}\n";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *stmt = func_body_stmt(ast, 0, 1);
        CHECK("while: NODE_WHILE",               stmt && stmt->kind == NODE_WHILE);
        CHECK("while: condition not null",        stmt && stmt->while_stmt.condition != NULL);
        CHECK("while: body not null",             stmt && stmt->while_stmt.body != NULL);
        CHECK("while: condition is binary",
              stmt && stmt->while_stmt.condition &&
              stmt->while_stmt.condition->kind == NODE_BINARY);
        CHECK("while: condition op is '<'",
              stmt && stmt->while_stmt.condition &&
              stmt->while_stmt.condition->binary.op == TOKEN_LT);
        CHECK("while: body has 1 stmt",
              stmt && stmt->while_stmt.body &&
              stmt->while_stmt.body->block.count == 1);
        ast_free(ast);
    }

    /* empty while body */
    {
        const char *src = "int f() { while (1) { } return 0; }";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *stmt = func_body_stmt(ast, 0, 0);
        CHECK("while empty body: NODE_WHILE",    stmt && stmt->kind == NODE_WHILE);
        CHECK("while empty body: 0 stmts",
              stmt && stmt->while_stmt.body &&
              stmt->while_stmt.body->block.count == 0);
        ast_free(ast);
    }

    /* nested while */
    {
        const char *src =
            "int f() {\n"
            "    while (a) { while (b) { x = 1; } }\n"
            "    return 0;\n"
            "}\n";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *outer = func_body_stmt(ast, 0, 0);
        CHECK("nested while: outer is NODE_WHILE", outer && outer->kind == NODE_WHILE);
        AstNode *inner_stmt = (outer && outer->while_stmt.body &&
                               outer->while_stmt.body->block.count > 0)
                              ? outer->while_stmt.body->block.stmts[0] : NULL;
        CHECK("nested while: inner is NODE_WHILE", inner_stmt && inner_stmt->kind == NODE_WHILE);
        ast_free(ast);
    }
}

static void test_call(void)
{
    printf("\n[call] — function calls\n");

    /* call as expression statement */
    {
        const char *src = "int f() { foo(); return 0; }";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *stmt = func_body_stmt(ast, 0, 0);
        CHECK("call no-args: NODE_CALL",    stmt && stmt->kind == NODE_CALL);
        CHECK("call no-args: 0 args",       stmt && stmt->call.args.count == 0);
        ast_free(ast);
    }

    /* call with arguments */
    {
        const char *src = "int f() { int r = add(1 + 2, 10); return r; }";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *stmt = func_body_stmt(ast, 0, 0);
        AstNode *init = stmt ? stmt->var_decl.init : NULL;
        CHECK("call 2-args: init is NODE_CALL",   init && init->kind == NODE_CALL);
        CHECK("call 2-args: 2 args",              init && init->call.args.count == 2);
        AstNode *arg0 = init ? init->call.args.items[0] : NULL;
        CHECK("call 2-args: arg0 is binary",      arg0 && arg0->kind == NODE_BINARY);
        AstNode *arg1 = init ? init->call.args.items[1] : NULL;
        CHECK("call 2-args: arg1 is number",      arg1 && arg1->kind == NODE_NUMBER);
        ast_free(ast);
    }

    /* nested call */
    {
        const char *src = "int f() { int r = foo(bar(1), 2); return r; }";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *stmt = func_body_stmt(ast, 0, 0);
        AstNode *call = stmt ? stmt->var_decl.init : NULL;
        CHECK("nested call: outer is NODE_CALL",  call && call->kind == NODE_CALL);
        CHECK("nested call: outer has 2 args",    call && call->call.args.count == 2);
        AstNode *inner = call ? call->call.args.items[0] : NULL;
        CHECK("nested call: arg0 is NODE_CALL",   inner && inner->kind == NODE_CALL);
        ast_free(ast);
    }
}

static void test_expr(void)
{
    printf("\n[expr] — expression precedence and associativity\n");

    /* 2 + 3 * 4 → + at root, * on right */
    {
        const char *src = "int f() { return 2 + 3 * 4; }";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *ret  = func_body_stmt(ast, 0, 0);
        AstNode *expr = ret ? ret->ret.value : NULL;
        CHECK("prec: top op is +",        expr && expr->binary.op == TOKEN_PLUS);
        CHECK("prec: right child is *",
              expr && expr->binary.right &&
              expr->binary.right->binary.op == TOKEN_STAR);
        ast_free(ast);
    }

    /* left-assoc: a + b + c → (a+b)+c */
    {
        const char *src = "int f() { return a + b + c; }";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *ret  = func_body_stmt(ast, 0, 0);
        AstNode *expr = ret ? ret->ret.value : NULL;
        CHECK("left-assoc: top op is +",      expr && expr->binary.op == TOKEN_PLUS);
        CHECK("left-assoc: left child is +",
              expr && expr->binary.left &&
              expr->binary.left->kind == NODE_BINARY &&
              expr->binary.left->binary.op == TOKEN_PLUS);
        ast_free(ast);
    }

    /* logical: a && b || c → (a&&b)||c */
    {
        const char *src = "int f() { return a && b || c; }";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *ret  = func_body_stmt(ast, 0, 0);
        AstNode *expr = ret ? ret->ret.value : NULL;
        CHECK("logical prec: top is ||",      expr && expr->binary.op == TOKEN_PIPEPIPE);
        CHECK("logical prec: left is &&",
              expr && expr->binary.left &&
              expr->binary.left->binary.op == TOKEN_AMPAMP);
        ast_free(ast);
    }

    /* unary minus */
    {
        const char *src = "int f() { return -x; }";
        AstNode *ast  = parse_and_check(src, 1);
        AstNode *ret  = func_body_stmt(ast, 0, 0);
        AstNode *expr = ret ? ret->ret.value : NULL;
        CHECK("unary: NODE_UNARY",    expr && expr->kind == NODE_UNARY);
        CHECK("unary: op is -",       expr && expr->unary.op == TOKEN_MINUS);
        CHECK("unary: operand is sym",
              expr && expr->unary.operand &&
              expr->unary.operand->kind == NODE_SYMBOL);
        ast_free(ast);
    }
}

/* ── suite registry ──────────────────────────────────────────────────────── */

typedef struct {
    const char *name;
    void (*fn)(void);
    const char *desc;
} Suite;

static Suite suites[] = {
    { "--func",   test_func,   "function definitions"     },
    { "--var",    test_var,    "variable declarations"    },
    { "--return", test_return, "return statements"        },
    { "--assign", test_assign, "assignment statements"    },
    { "--if",     test_if,     "if / else-if / else"      },
    { "--while",  test_while,  "while loops"              },
    { "--call",   test_call,   "function call expressions"},
    { "--expr",   test_expr,   "expression precedence"    },
};

#define SUITE_COUNT (sizeof suites / sizeof *suites)

static void print_usage(const char *argv0)
{
    printf("Usage:\n");
    printf("  %s              run all tests\n", argv0);
    printf("  %s --list       list available flags\n", argv0);
    for (size_t i = 0; i < SUITE_COUNT; i++)
        printf("  %s %-10s  %s\n", argv0, suites[i].name, suites[i].desc);
}

int main(int argc, char **argv)
{
    printf("=== parser test suite ===\n");

    if (argc == 2 && strcmp(argv[1], "--list") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    if (argc == 1) {
        /* no flags → run everything */
        for (size_t i = 0; i < SUITE_COUNT; i++)
            suites[i].fn();
    } else {
        /* run only the suites whose flag appears in argv */
        int found = 0;
        for (int a = 1; a < argc; a++) {
            int matched = 0;
            for (size_t i = 0; i < SUITE_COUNT; i++) {
                if (strcmp(argv[a], suites[i].name) == 0) {
                    suites[i].fn();
                    matched = 1;
                    found   = 1;
                    break;
                }
            }
            if (!matched) {
                fprintf(stderr, "unknown flag '%s'\n", argv[a]);
                print_usage(argv[0]);
                return 1;
            }
        }
        if (!found) return 1;
    }

    printf("\n=========================\n");
    printf("run: %d  pass: %d  fail: %d\n", tests_run, tests_passed, tests_failed);
    printf("=========================\n");
    return tests_failed > 0 ? 1 : 0;
}
