#include <stdio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

int debugging_printf = 1;
static uint64_t break_start, break_end;
static const uint64_t break_size = 0x100000;

int64_t syscall_handler (int64_t syscall_number,
        int64_t p0, int64_t p1, int64_t p2, int64_t p3, int64_t p4)
{
    switch (syscall_number) {
        case 0x10:
            if (debugging_printf) {
                printf ("ioctl fd1 %jx cmd %jx ptr %jx\n", p0, p1, p2);
            }
            return ioctl (p0, p1, p2, p3, p4);
        case 1:
            return write (p0, (const void *) p1, p2);
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
        default:
            printf ("Unsupported system call %jx p0 %jx p1 %jx p2 %jx\n",
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
    FILE *          fd1;
    FILE *          fd2;
    uint64_t *      p0;
    uint64_t *      p1;
    uint64_t *      p2;
    uint64_t        load_size;
    uint64_t        fini_address;
    uint64_t        end_address;
    int             rc;
    uint64_t        i;
    struct termios  ignore;

    tcgetattr (0, &ignore);

    printf ("argc = %d\n", argc);
    printf ("argv = %p\n", argv);
    printf ("argv[0] = %p\n", argv[0]);

    fd1 = fopen ("example.bin", "rb");
    assert (fd1 != NULL);
    rc = fseek (fd1, 0, SEEK_END);
    assert (rc == 0);
    load_size = ftell (fd1);
    rc = fseek (fd1, 0, SEEK_SET);
    assert (rc == 0);

    printf ("fd1 = %p, fileno = %d\n", fd1, fileno (fd1));

    printf ("size = %jx\n", load_size);
    if (load_size < 0x100) {
        printf ("too small\n");
        return 1;
    }

    fd2 = fopen ("example.off", "rb");
    assert (fd2 != NULL);
    rc = fseek (fd2, 0, SEEK_END);
    assert (rc == 0);
    assert (load_size == ftell (fd2));
    rc = fseek (fd2, 0, SEEK_SET);
    assert (rc == 0);

    p1 = mmap (NULL, load_size, PROT_READ,
              MAP_PRIVATE, fileno (fd1), 0);
    assert (p1 != NULL);
    printf ("p1 = %p\n", p1);

    p2 = mmap (NULL, load_size, PROT_READ,
              MAP_PRIVATE, fileno (fd2), 0);
    assert (p2 != NULL);
    printf ("p2 = %p\n", p2);

    fini_address = p1[3];
    end_address = p1[4];

    printf ("_fini = %jx\n", fini_address);
    if (fini_address > load_size) {
        printf ("_fini beyond EOF\n");
        return 1;
    }
    printf ("_end = %jx\n", end_address);
    if (end_address < load_size) {
        printf ("_end not beyond EOF\n");
        return 1;
    }

    p0 = mmap (NULL, end_address + break_size,
              PROT_READ | PROT_WRITE | PROT_EXEC,
              MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    assert (p0 != NULL);
    printf ("p0 = %p\n", p0);

    break_start = ((uint64_t) p0) + end_address;
    break_end = ((uint64_t) p0) + end_address + break_size;

    memset (p0, 0, end_address);
    memcpy (p0, p1, load_size);

    for (i = (fini_address / 8); i < (load_size / 8); i ++) {
        uint64_t diff = p2[i] - p1[i];
        if (diff != 0) {
            if (diff != 0x12340000U) {
                printf ("0x%05jx: 0x%016jx 0x%016jx difference 0x%016jx\n",
                        i * 8, p1[i], p2[i], diff);
                return 1;
            }
            p0[i] = p1[i] + (uint64_t) p0;
        }
    }
    munmap (p2, load_size);
    fclose (fd2);
    munmap (p1, load_size);
    fclose (fd1);

    p0[2] = (uint64_t) syscall_handler;
    printf ("syscall_handler = %016jx\n", p0[2]);
    printf ("launch = %p\n\n\n\n", p0);

    go_shim (p0, argc, argv);
    munmap (p0, load_size);
    return 0;
}

