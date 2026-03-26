#!/usr/bin/env bash

gcc -O3 -std=c99 \
    -Werror -Wall -Wextra -pedantic \
    -Wno-declaration-after-statement \
    -o csfm main.c
