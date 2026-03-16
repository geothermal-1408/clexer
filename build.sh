#!/bin/sh

set -xe

#CC=${CC:-cc}
CC="clang"
CFLAGS="-Wall -Wextra -std=c11 -pedantic -O2"

SRC="*.c"
# OBJ="lexer.o"
# LIB="liblexer.a"

build() {
    # compile source to object file
    for file in $SRC; do
	base_name="${file%.c}"
	obj_file="${base_name}.o"
	lib_file="lib${base_name}.a"
	
	$CC $CFLAGS -c "$file" -o "$obj_file"
	ar rcs "$lib_file" "$obj_file"
    done
}

test() {
    mkdir -p bin
    $CC $CFLAGS -o bin/usage usage.c -L. -llexer 
}

parser() {
    mkdir -p bin
    $CC $CFLAGS $SRC test/parser_test.c -I. -o bin/parser_test -L. -llexer -lparser
}
case "$1" in
    build)
        build
        ;;
    test)
        test
        ;;
    parser)
        parser
        ;;
    *)
        echo "Usage: $0 {build|test}"
        exit 1
        ;;
esac
