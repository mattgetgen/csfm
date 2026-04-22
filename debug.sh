#!/usr/bin/env bash

gcc -O0 -std=c99 \
    -Wall -Wextra -pedantic \
    -fsanitize=address -fsanitize=undefined \
    -o csfm main.c
