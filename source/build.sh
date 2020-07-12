#!/bin/sh

pushd ../target/debug
[ -f "colors.o" ] && rm colors.o
[ -f "draw.o" ] && rm draw.o
[ -f "twenty_fortyeight.o" ] && rm twenty_fortyeight.o
[ -f "tf" ] && rm tf

gcc -Wall -g -c ../../source/colors.c
gcc -Wall -g -c ../../source/draw.c
gcc -Wall -g -c ../../source/twenty_fortyeight.c
gcc -lX11 -g -o tf colors.o draw.o twenty_fortyeight.o
popd
