#!/usr/bin/env bash

gcc -std=c89 -Wall -Wextra -pedantic -Wno-declaration-after-statement -fsanitize=address -o csfm main.c
