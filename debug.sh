#!/usr/bin/env bash

gcc -O0 -std=c99 \
    -Werror -Wall -Wextra -pedantic \
    -Wno-unused-function \
    -Wno-unused-parameter \
    -Wno-declaration-after-statement \
    -fsanitize=address -fsanitize=undefined \
    -o csfm main.c
