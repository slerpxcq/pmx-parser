#!/bin/sh

cflags="-Wall -pedantic -std=gnu11 -ggdb"
gcc main.c -o main.o ${cflags}
gcc pmx_model.c -o pmx_model.o ${cflags}
gcc main.o pmx_model.o -o main
