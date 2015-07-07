#include <stdio.h>
#include <windows.h>
#include <assert.h>

#include "load.h"

void go_shim (uint64_t * p1, int argc, char ** argv, uint8_t * sp);
void syscall_shim (void);

int main (int argc, char ** argv)
{
    uint64_t *      p0;
    uint8_t *       stack;
    const char *    source_fname;
    size_t          embed_minimum_size;

    if (argc < 2) {
        fputs ("Usage: winloader.exe <program.prg> [args...]\n", stderr);
        return 1;
    }

    source_fname = argv[1];

    embed_minimum_size = init_load (source_fname);
    p0 = VirtualAlloc (NULL, embed_minimum_size + MAX_HEAP_SIZE,
                 MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    assert (p0 != NULL);
    printf ("program and heap %p .. %p\n",
        p0,
        ((uint8_t *) p0) + embed_minimum_size + MAX_HEAP_SIZE);

    stack = VirtualAlloc (NULL, MAX_STACK_SIZE,
                 MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    assert (stack != NULL);
    printf ("stack %p .. %p\n", stack, stack + MAX_STACK_SIZE);

    init_syscall
        ((((uint64_t) p0) + embed_minimum_size),
         (((uint64_t) p0) + embed_minimum_size + MAX_HEAP_SIZE));
    do_load (p0, syscall_shim, source_fname);

    printf ("launching\n\n\n\n");

    go_shim (p0, argc - 1, &argv[1], stack + MAX_STACK_SIZE - 0x10);
    /* failure...! */
    return 1;
}

