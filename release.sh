#!/usr/bin/env bash

gcc -O3 -std=c99 \
    -Werror -Wall -Wextra -pedantic \
    -o csfm main.c
