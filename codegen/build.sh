#!/usr/bin/env bash

set -e

CC=gcc
CFLAGS=(
    -std=c99
    -Wall
    -Wextra
    -pedantic
    -fshort-enums
    -O0
    -g
    -fsanitize=address
    -fsanitize=undefined
)

$CC "${CFLAGS[@]}" codegen.c -o csfm_codegen

