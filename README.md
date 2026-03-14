# C Lexer

A simple C Lexer designed to break down source code into a stream of tokens.

## Features

### Tokens

- **Single-character tokens**: `( ) { } ; , < > . # + - * / % & | ^ ~ !`
- **Multi-character tokens**:
  - Equality & relational: `== != <= >=`
  - Logical: `&& ||`
  - Bitwise shift: `<< >>`
  - Increment / decrement: `++ --`
  - Compound assignment: `+= -= *= /= %= &= |= ^= <<= >>=`
  - Member access: `->`
- **Literals**: Integer numbers and double-quoted strings (with escape sequence awareness).
- **Symbols**: Identifiers and keywords (`int`, `return`, `if`).

### Lexer Behaviour

- **Whitespace handling**: Skips spaces, tabs, and newlines.
- **Comment handling**: Skips both line comments (`//`) and block comments (`/* */`).
- **Maximal munch**: Always lexes the longest valid token (`<<=` before `<<` before `<`).
- **Source location tracking**: Every token carries the `line` and `col` of its first character.
- **Error recovery**: Unrecognised characters emit `TOKEN_INVALID` and the lexer advances cleanly to the next character.

## Build

```console
chmod +x build.sh
./build.sh
```

## Usage

```console
./usage <filename.c>
```

## Test

The test suite contains 3 files:

- `hello.c` and `test2.c` are used for lexer output and performance benchmarking.

```console
./usage --{bench} test/<file_name>
```

- `test3.c` is a self-contained unit test file that reports pass/fail counts for all token kinds and edge cases.

```console
clang -std=c11 test/test3.c -I. -L. -llexer -o test/test3
./test/test3
```

> [!NOTE]
>
> **DISCLAIMER:** This is currently in development phase. It is also not very ergonomic right now.
