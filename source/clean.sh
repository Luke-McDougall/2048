#!/bin/sh

pushd ../target/release
[ -f "colors.o" ] && rm colors.o
[ -f "draw.o" ] && rm draw.o
[ -f "twenty_fortyeight.o" ] && rm twenty_fortyeight.o
[ -f "tf" ] && rm tf
popd

pushd ../target/debug
[ -f "colors.o" ] && rm colors.o
[ -f "draw.o" ] && rm draw.o
[ -f "twenty_fortyeight.o" ] && rm twenty_fortyeight.o
[ -f "tf" ] && rm tf
popd
