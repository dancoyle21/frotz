#!/bin/bash -xe

make -C uClibc-0.9.33.2
make -C frotz src/frotz_common.a src/frotz_dumb.a
# gcc -o dfrotz src/frotz_common.a src/frotz_dumb.a 

cd loader

as -o setup.o setup.s 

OBJS="setup.o ../frotz/src/frotz_common.a ../frotz/src/frotz_dumb.a \
        ../uClibc-0.9.33.2/lib/libc.a"
ld -o unoffset.elf -Tunoffset.x $OBJS
ld -o offset.elf -Toffset.x $OBJS

objcopy -O binary unoffset.elf unoffset.bin
objcopy -O binary offset.elf offset.bin

python make_program.py

gcc -o linuxloader.exe linuxloader.c load.c syscall.c -Wall -g

cp program linuxloader.exe ..

cd ..

cp ~/Downloads/vgame.z8 . || true
genisoimage -o demo.iso -r -J \
    TempleLoader.CPP Frotz.CPP *.z8 *.z5 program README*

# on windows...
# gcc -m64 -o winloader.exe winloader.c load.c syscall.c winshims.s -Wall -g

