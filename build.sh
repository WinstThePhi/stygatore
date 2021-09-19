#!/bin/bash

echo "gen_struct.c"
clang -g code/gen_struct.c -o build/gen_struct.out
