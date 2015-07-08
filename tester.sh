#!/bin/bash -xe

make -C uClibc-0.9.33.2
cd tester
as -o main.o main.s
cd ..

loader/build.sh tester.prg tester/main.o


