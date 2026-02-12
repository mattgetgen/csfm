#!/usr/bin/env bash

# debug build
gcc -O0 -std=c89 -Werror -Wall -Wextra -pedantic -Wno-declaration-after-statement -fsanitize=address -fsanitize=undefined -o csfm main.c
# perf build
# gcc -O3 -std=c89 -Werror -Wall -Wextra -pedantic -Wno-declaration-after-statement -o csfm main.c
