#!/bin/bash -xe

make src/frotz_common.a src/frotz_dumb.a
# gcc -o dfrotz src/frotz_common.a src/frotz_dumb.a 

as -o setup.o setup.s 

OBJS="setup.o src/frotz_common.a src/frotz_dumb.a \
        uClibc-0.9.33.2/lib/libc.a"
ld -o unoffset.elf -Tunoffset.x $OBJS
ld -o offset.elf -Toffset.x $OBJS

objcopy -O binary unoffset.elf unoffset.bin
objcopy -O binary offset.elf offset.bin

python make_program.py

gcc -o linuxloader.exe linuxloader.c load.c syscall.c -Wall -g

# on windows...
# gcc -m64 -o winloader.exe winloader.c load.c syscall.c winshims.s -Wall -g

