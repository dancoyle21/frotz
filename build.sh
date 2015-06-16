#!/bin/bash -xe

make src/frotz_common.a src/frotz_dumb.a
# gcc -o dfrotz src/frotz_common.a src/frotz_dumb.a 

as -o example1.o example1.s 

OBJS="example1.o src/frotz_common.a src/frotz_dumb.a \
        uClibc-0.9.33.2/lib/libc.a"
ld -o example.elf -Texample.x $OBJS
ld -o offset.elf -Toffset.x $OBJS

objcopy -O binary example.elf example.bin
objcopy -O binary offset.elf example.off

python embed.py

gcc -o loader.exe loader.c embed.c syscall.c -Wall -g
