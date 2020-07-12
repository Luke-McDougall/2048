#!/bin/sh

pushd ../target/release
[ -f "colors.o" ] && rm colors.o
[ -f "draw.o" ] && rm draw.o
[ -f "twenty_fortyeight.o" ] && rm twenty_fortyeight.o
[ -f "tf" ] && rm tf

gcc -Wall -O3 -c ../../source/colors.c
gcc -Wall -O3 -c ../../source/draw.c
gcc -Wall -O3 -c ../../source/twenty_fortyeight.c
gcc -lX11 -O3 -o tf colors.o draw.o twenty_fortyeight.o
popd
