#!/usr/bin/env bash

gcc -O0 -std=c89 \
    -Werror -Wall -Wextra -pedantic \
    -Wno-unused-function \
    -Wno-unused-parameter \
    -Wno-declaration-after-statement \
    -fsanitize=address -fsanitize=undefined \
    -o csfm main.c
