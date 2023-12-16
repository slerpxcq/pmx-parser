#!/bin/sh
clear && gcc parser.c -Iinc -o parser -Wall -pedantic -std=gnu11 -ggdb && ./parser
