#ifndef EMBED_H
#define EMBED_H

#include <stdint.h>

#define MAX_HEAP_SIZE 0x100000

extern const size_t embed_minimum_size;
extern void embed_copy (uint64_t * destination, void * syscall_handler);

int64_t syscall_handler (int64_t syscall_number,
        int64_t p0, int64_t p1, int64_t p2);
void init_syscall (uint64_t _break_start, uint64_t _break_end);


#endif

