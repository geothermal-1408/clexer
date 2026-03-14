#!/bin/sh

set -xe

#CC=${CC:-cc}
CC="clang"
CFLAGS="-Wall -Wextra -std=c11 -pedantic -O2"

SRC="lexer.c"
OBJ="lexer.o"
LIB="liblexer.a"

build() {
    # compile source to object file
    $CC $CFLAGS -c $SRC -o $OBJ

    # create static library
    ar rcs $LIB $OBJ
}

test() {
    $CC $CFLAGS -o bin/usage usage.c -L. -llexer
}

case "$1" in
    build)
        build
        ;;
    test)
        test
        ;;
    *)
        echo "Usage: $0 {build|test}"
        exit 1
        ;;
esac
