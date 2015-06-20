#ifndef LOAD_H
#define LOAD_H

#include <stdint.h>

#define MAX_HEAP_SIZE 0x100000

int64_t syscall_handler (int64_t syscall_number,
        int64_t p0, int64_t p1, int64_t p2);
void init_syscall (uint64_t _break_start, uint64_t _break_end);

size_t init_load (const char * source_fname);
void do_load (uint64_t * destination, void * syscall_handler,
              const char * source_fname);

#endif

