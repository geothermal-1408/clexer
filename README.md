# C Lexer

A simple C Lexer designed to break down source code into a stream of tokens.

## Features

- **Single-character tokens**: `( ) { } ; , < > . #`
- **Multi-character tokens**: `==`
- **Literals**: Numbers and double-quoted strings.
- **Symbols**: Identifiers and keywords.
- **Whitespace Handling**: Skips spaces, tabs, and newlines.

## Build

```console
chmod +x build.sh
./build.sh
```

## Usage

```console
./usage <filename.c>
```

> [!NOTE]
>
> **DISCLAIMER:** This is in cuurently in development phase. Tested with "hello world".
