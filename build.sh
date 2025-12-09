#!/usr/bin/env bash

gcc -std=c89 -Wall -Wextra -fsanitize=address -o csfm main.c
