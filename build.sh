#!/usr/bin/env bash

gcc -std=c89 -Wall -Wextra -Wpedantic -fsanitize=address -o csfm main.c
