#include <stdio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#include "uClibc-0.9.33.2/libc/termios/kernel_termios.h"

#define max_heap_size 0x100000
#define max_fd_handles 0x10

extern const size_t embed_minimum_size;
extern void embed_copy (uint64_t * destination);

static int debugging_printf = 1;
static uint64_t break_start, break_end;
static FILE * fd_handle[max_fd_handles];

static int64_t do_ioctl (int64_t p0, int64_t p1, int64_t p2)
{
    struct __kernel_termios * k;
    switch (p1) {
        case 0x5401:
            /* TCGETS */
            k = (struct __kernel_termios *) p2;
            memset (k, 0, sizeof (struct __kernel_termios));
            return 0;
        default:
            printf ("Unsupported ioctl %ju p1 %jx p2 %jx\n",
                    p0, p1, p2);
            exit (1);
            return 0;
    }
}

static int64_t do_open (int64_t p0, int64_t p1)
{
    unsigned i;

    if ((p1 & O_WRONLY) || (p1 & O_RDWR)) {
        printf ("Unsupported open mode for '%s': 0x%jx\n",
                (const char *) p0, p1);
        exit (1);
    } else {
        FILE * fd = fopen ((const char *) p0, "rb");
        if (fd == NULL) {
            /* file not found, perhaps */
            return -1;
        }
        for (i = 0; i < max_fd_handles; i++) {
            if (fd_handle[i] == NULL) {
                fd_handle[i] = fd;
                fd = NULL;
                return i;
            }
        }
        printf ("No handles available for '%s'\n",
                (const char *) p0);
        exit (1);
    }
    return -1;
}

int64_t syscall_handler (int64_t syscall_number,
        int64_t p0, int64_t p1, int64_t p2, int64_t p3, int64_t p4)
{
    switch (syscall_number) {
        case 0x10:
            return do_ioctl (p0, p1, p2);
        case 2:
            return do_open (p0, p1);
        case 0:
        case 1:
        case 3:
            switch (p0) {
                case STDIN_FILENO:
                    return read (0, (void *) p1, p2);
                case STDOUT_FILENO:
                case STDERR_FILENO:
                    return write (1, (void *) p1, p2);
                default:
                    break;
            }
            if ((p0 <= 0) || (p0 >= (int64_t) max_fd_handles)) {
                printf ("Invalid handle: 0x%jx\n", p0);
                exit (1);
            }
            switch (syscall_number) {
                case 0:
                    fflush (stdout);
                    p0 = fread ((void *) p1, 1, p2, fd_handle[p0]);
                    break;
                case 1:
                    p0 = fwrite ((void *) p1, 1, p2, fd_handle[p0]);
                    fflush (fd_handle[p0]);
                    break;
                default:
                    fclose (fd_handle[p0]);
                    fd_handle[p0] = NULL;
                    p0 = 0;
                    break;
            }
            return p0;
        case 60:
            if (debugging_printf) {
                printf ("exit\n");
            }
            exit (p0);
            return 1;
        case 12:
            if (p0 < break_start) {
                p0 = break_start;
            }
            if (p0 > break_end) {
                p0 = break_end;
            }
            if (debugging_printf) {
                printf ("brk %016jx (%ju bytes)\n", p0, p0 - break_start);
            }
            return p0;
        case 201:
            /* time */
            return 1400000000;
        default:
            printf ("Unsupported system call %ju p0 %jx p1 %jx p2 %jx\n",
                    syscall_number, p0, p1, p2);
            exit (1);
            return 0;
    }
}

void go_shim (uint64_t * p1, int argc, char ** argv)
{
    void (* go) (int argc, char ** argv);
    go = (void *) (p1);
    go (argc, argv);
}

int main (int argc, char ** argv)
{
    uint64_t *      p0;

    fd_handle[0] = stdin;
    fd_handle[1] = stdout;
    fd_handle[2] = stderr;

    printf ("argc = %d\n", argc);
    printf ("argv = %p\n", argv);
    printf ("argv[0] = %p\n", argv[0]);


    p0 = mmap (NULL, embed_minimum_size + max_heap_size,
              PROT_READ | PROT_WRITE | PROT_EXEC,
              MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    assert (p0 != NULL);
    printf ("p0 = %p\n", p0);

    break_start = ((uint64_t) p0) + embed_minimum_size;
    break_end = ((uint64_t) p0) + embed_minimum_size + max_heap_size;
    embed_copy (p0);

    p0[2] = (uint64_t) syscall_handler;
    printf ("syscall_handler = %016jx\n", p0[2]);
    printf ("launch = %p\n\n\n\n", p0);

    go_shim (p0, argc, argv);
    /* failure...! */
    return 1;
}

