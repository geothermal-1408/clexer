#!/bin/sh

set -xe

#CC=${CC:-cc}
CC="clang"
CFLAGS="-Wall -Wextra -std=c11 -pedantic -O2"

SRC="lexer.c"
OBJ="lexer.o"
LIB="liblexer.a"

# compile source to object file
$CC $CFLAGS -c $SRC -o $OBJ

# create static library
ar rcs $LIB $OBJ
