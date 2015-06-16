#!/usr/bin/python

import struct


def read_qword(data, offset):
    offset *= 8
    qword = data[offset : offset + 8]
    (value, ) = struct.unpack("<Q", qword)
    return value

def main(binary_fname, offset_fname, c_fname):

    binary_data = open(binary_fname, "rb").read()
    offset_data = open(offset_fname, "rb").read()

    assert len(binary_data) == len(offset_data)
    assert binary_data != offset_data

    # examine input
    load_size = len(binary_data)
    fini_address = read_qword(binary_data, 3)
    end_address = read_qword(binary_data, 4)

    assert fini_address <= load_size, "_fini beyond EOF"
    assert end_address >= load_size, "end not beyond EOF"

    # create C output
    c_out = open(c_fname, "wt")
    c_out.write("""
#include <string.h>
#include <stdint.h>

/* external */
const size_t embed_minimum_size = %u;
void embed_copy (uint64_t * destination, void * syscall_handler);

/* internal */
static const size_t load_size = %u;
static const unsigned char base_data[] =
""" % (end_address, load_size))
   
    pad = "\x00" * 32
    binary_data += pad
    offset_data += pad

    # copy base data
    for i in xrange(load_size / 16):
        c_out.write('"')
        for j in xrange(i * 16, (i + 1) * 16):
            c_out.write('\\x%02x' % ord(binary_data[j]))
        c_out.write('"\n')
    c_out.write(""";
static const unsigned reloc_data[] = {
""")

    # detect locations of pointers and add relocations
    interval = 6
    for i in xrange(fini_address / 8, load_size / 8, 1):
        diff = read_qword(offset_data, i) - read_qword(binary_data, i)
        if diff != 0:
            assert diff == 0x12340000, ("diff unexpected value", i)
            c_out.write(" 0x%06x," % i)
            interval -= 1
            if interval <= 0:
                c_out.write("\n")
                interval = 6

    # the C code that does the work
    c_out.write(""" 0x0};

void embed_copy (uint64_t * destination, void * syscall_handler)
{
    uint64_t reloc = ((uint64_t) destination);
    unsigned i;

    memset (destination, 0, embed_minimum_size);
    memcpy (destination, base_data, load_size);

    for (i = 0; reloc_data[i] != 0; i++) {
        destination[reloc_data[i]] += reloc;
    }
    destination[2] = (uint64_t) syscall_handler;
}
""")

    c_out.close()

if __name__ == "__main__":
    main(binary_fname = "example.bin",
         offset_fname = "example.off",
         c_fname = "embed.c")

