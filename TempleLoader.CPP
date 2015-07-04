
#define TL_MAX_HEAP_SIZE    0x100000
#define TL_MAX_FDS          100
#define TL_SIZE_LIMIT       0x10000
#define TL_ALIGN_SIZE       0x1000
#define TL_PROGRAM          "program"

U64 * TL_Base;
U64 * TL_Argv;
U64 * TL_Restore_SP;
U64 TL_Argc;
U64 TL_Break_Start;
U64 TL_Break_End;
U64 TL_Program_Start;
U64 TL_Program_End;


class TL_FD
{
    U8 * base;
    U64 remaining;
    U64 position;
};

TL_FD TL_FD_Handle[TL_MAX_FDS];

class TL_ProgramHeader
{
    U64 start;
    U64 version;
    U64 syscall_address;
    U64 fini_address;
    U64 end_address;
    U64 load_size;
    U64 total_size;
    U8 pad[BLK_SIZE-(7*8)];
};

asm {
    ALIGN   8,OC_NOP
_TL_RESTORE_POINT::

    MOV     RCX,I64 [&TL_Restore_SP] MOV     RSP,RCX

    POP_REGS

    POP     RBP
    RET

}
_extern _TL_RESTORE_POINT U0 TL_Restore_Point();

U8 * TL_To_Ptr (U64 p0)
{
    U8 * rc;
    MemCpy (&rc, &p0, 8);
    return rc;
}

U64 TL_To_Int (U8 * p0)
{
    U64 rc;
    MemCpy (&rc, &p0, 8);
    return rc;
}

U64 TL_Syscall_Ioctl (U64 p0, U64 p1, U64 p2)
{
    switch (p1) {
        case 0x5401:
            /* TCGETS */
            return 0;
        default:
            "\n\nunsupported ioctl reached, p0 = %d, p1 = %d, p2 = %d\n",
                p0, p1, p2;
            TL_Restore_Point;
            return 0;
    }
}

U64 TL_Syscall_Read (U64 fd, U8 * buf, U64 size)
{
    if ((size == 0) || (size > TL_SIZE_LIMIT)) {
        "\n\nunsupported read of size %d to fd %d\n", size, fd;
        TL_Restore_Point;
    }
    if (fd == 0) {
        // read from stdin
        buf[0] = GetChar (NULL, FALSE, FALSE);
        '' buf[0];
        return 1;
    }
    // read from some other file handle (check the handle first)
    if ((fd >= TL_MAX_FDS) || (TL_FD_Handle[fd].base == NULL)) {
        "\n\nunsupported read from fd %d\n", fd;
        TL_Restore_Point;
    }
    // bound read by file size
    if (size > TL_FD_Handle[fd].remaining) {
        size = TL_FD_Handle[fd].remaining;
    }
    // do read (copy)
    MemCpy (buf, TL_FD_Handle[fd].base + TL_FD_Handle[fd].position, size);

    // advance pointers
    TL_FD_Handle[fd].remaining -= size;
    TL_FD_Handle[fd].position += size;
    "read %d bytes from fd %d to %p\n", size, fd, buf;
    return size;
}

U64 TL_Syscall_Write (U64 fd, U8 * buf, U64 size)
{
    U64 i;
    if ((fd != 1) && (fd != 2)) {
        "\n\nunsupported write to fd %d\n", fd;
        TL_Restore_Point;
    }
    if ((size == 0) || (size > TL_SIZE_LIMIT)) {
        "\n\nunsupported write of size %d to fd %d\n", size, fd;
        TL_Restore_Point;
    }
    for (i = 0; i < size; i++) {
        if (buf[i] == 0x0a) {
            "\n";
        } else if (buf[i] != 0x0d) {
            '' buf[i];
        }
    }
    return size;
}

U64 TL_Syscall_Open (U8 * buf, U64 mode)
{
    CFile * f;
    I64 rc, i;
    U64 num_blocks;

    if ((mode & 3) != 0) {
        "\n\nunsupported open mode for file '%s': %d\n", buf, mode;
        TL_Restore_Point;
    }
    f = FOpen (buf, "r");
    if (!f) {
        // ENOENT
        return -2;
    }
    rc = -24; // EMFILE
    for (i = 3; i < TL_MAX_FDS; i++) {
        if (TL_FD_Handle[i].base == NULL) {
            rc = i;
            break;
        }
    }
    if (rc < 0) {
        FClose (f);
        return rc; // EMFILE
    }
    TL_FD_Handle[rc].position = 0;
    TL_FD_Handle[rc].remaining = FSize (f);
    if (TL_FD_Handle[rc].remaining == 0) {
        "\n\nunsupported open empty file '%s'\n", buf;
        TL_Restore_Point;
    }

    num_blocks = (TL_FD_Handle[rc].remaining + BLK_SIZE - 1) / BLK_SIZE;
    TL_FD_Handle[rc].base = MAlloc (num_blocks * BLK_SIZE);

    for (i = 0; i < num_blocks; i++) {
        U8 * file_entry = TL_FD_Handle[rc].base + (i * BLK_SIZE);
        if (!FRBlks (f, file_entry, i, 1)) {
            "\n\nunable to read block %d from file '%s'\n", i, buf;
            return FALSE;
        }
    }
    FClose (f);
    "open '%s' -> fd %d\n", buf, rc;
    return rc;
}

U64 TL_Syscall_Close (U64 fd)
{
    if ((fd >= TL_MAX_FDS) || (fd <= 2) || (TL_FD_Handle[fd].base == NULL)) {
        "\n\nunsupported close for fd %d\n", fd;
        TL_Restore_Point;
    }
    Free (TL_FD_Handle[fd].base);
    TL_FD_Handle[fd].base = NULL;
    TL_FD_Handle[fd].position = 0;
    TL_FD_Handle[fd].remaining = 0;
    return 0;
}

U64 TL_Syscall_Brk (U64 pos)
{
    // brk (allocate memory)
    if (pos < TL_Break_Start) {
        pos = TL_Break_Start;
    }
    if (pos > TL_Break_End) {
        pos = TL_Break_End;
    }
    return pos;
}

U0 TL_Register_Dump (U64 * rax)
{
    U64 i, j, p;
    j = 0;
    for (i = 15; i >= 8; i--) {
        "r%d = %08x\n", i, rax[j];
        j++;
    }
    "rdi = %08x\n", rax[j]; j++;
    "rsi = %08x\n", rax[j]; j++;
    "rbp = %08x\n", rax[j]; j++;
    "rbx = %08x\n", rax[j]; j++;
    "rdx = %08x\n", rax[j]; j++;
    "rcx = %08x\n", rax[j]; j++;
    "rax = %08x\n", rax[j]; j++;
    "return %08x -> %08x\n", rax[j], rax[j] - TL_Program_Start;
    "rsp %p\n", &rax[j]; j++;
    for (i = 0; i < 2048; i++) {
        p = rax[j]; j++;
        if ((p > TL_Program_Start) && (p < TL_Program_End)) {
            "above[%d] = %08x -> %08x\n", i, p, p - TL_Program_Start;
        }
    }
}

U64 TL_Syscall(U64 number, U64 p0, U64 p1, U64 p2, U64 * rax)
{
    switch (number) {
        case 0:
            return TL_Syscall_Read (p0, TL_To_Ptr (p1), p2);
        case 1:
            return TL_Syscall_Write (p0, TL_To_Ptr (p1), p2);
        case 2:
            return TL_Syscall_Open (TL_To_Ptr (p0), p1);
        case 3:
            return TL_Syscall_Close (p0);
        case 12:
            return TL_Syscall_Brk (p0);
        case 16:
            return TL_Syscall_Ioctl (p0, p1, p2);
        case 60:
            "\n\nexit\n";
            TL_Restore_Point;
            return 0;
        case 201:
            // time
            return 1400000000;
        default:
            "\n\nunsupported system call reached! number = %d\n", number;
            "p0 = %x (%d), p1 = %x (%d), p2 = %x (%d)\n",
                p0, p0, p1, p1, p2, p2;
            TL_Register_Dump (rax);
            TL_Restore_Point;
            return 0;
    }
    return 0;
}

asm {
_TL_LAUNCH::
    PUSH    RBP
    MOV     RBP, RSP

    PUSH_REGS

    MOV     RCX,RSP
    MOV     I64 [&TL_Restore_SP], RCX
    MOV     RDI,I64 [&TL_Argc]
    MOV     RSI,I64 [&TL_Argv]
    MOV     RCX,I64 [&TL_Base]

    CALL    RCX

    HLT

    ALIGN   8,OC_NOP
_TL_SYSCALL_POINT::

    PUSH_REGS
    // TempleOS: push args on stack in right-to-left order
    //           Called function cleans up the stack
    // Linux: args in registers (l->r): RDI, RSI, RDX, RCX
    // Both: returned value in RAX
   
    MOV     RAX, RSP
    PUSH    RAX
    PUSH    RCX
    PUSH    RDX
    PUSH    RSI
    PUSH    RDI
    CALL    &TL_Syscall

    // returned value will be restored by POP_REGS:
    // RSP[0*8] is r15
    // RSP[1*8] is r14
    // ...
    // RSP[14*8] is rax
    // 14*8 = 112
    MOV     I64 [RSP+112],RAX
    POP_REGS
    RET
    ALIGN   8,OC_NOP
}

_extern _TL_LAUNCH U0 TL_Launch();
_extern _TL_SYSCALL_POINT U0 TL_Syscall_Point();

Bool TL_Loader(...)
{
    TL_ProgramHeader header;
    CFile *f;
    U64 num_blocks;
    U64 offset;
    U8 * base_copy;
    U8 * true_base;
    U8 * table_entry;
    U8 * file_entry;
    U8 * program;
    U64 i, tmp;
    U32 reloc;

    if (argc < 1) {
        "Pass the program file name to TL_Loader\n";
        return FALSE;
    }
    program = TL_To_Ptr (argv[0]);

    // read first block
    f = FOpen (program, "r");
    if (!f) {
        "Unable to open '%s' file\n\n", program;
        return FALSE;
    }
    if (!FRBlks (f, &header, 0, 1)) {
        "Unable to read header from '%s' file\n\n", program;
        return FALSE;
    }
    "program header:\n";
    "fini_address %d.\n", header.fini_address;
    "end_address %d.\n", header.end_address;
    "load_size %d.\n", header.load_size;
    "total_size %d.\n", header.total_size;

    if ((header.start >> 16) != 0x77646a000000) {
        FClose (f);
        "This program file is not valid.\n";
        return FALSE;
    }

    num_blocks = (header.total_size + BLK_SIZE - 1) / BLK_SIZE;

    "number of blocks %d.\n", num_blocks;

    // allocate
    true_base = MAlloc (TL_MAX_HEAP_SIZE + header.total_size + TL_ALIGN_SIZE);
    MemSet (true_base, 0x0,
            TL_MAX_HEAP_SIZE + header.total_size + TL_ALIGN_SIZE);

    // round up to page
    offset = TL_To_Int (true_base);
    offset = (offset + TL_ALIGN_SIZE - 1) / TL_ALIGN_SIZE;
    offset *= TL_ALIGN_SIZE;
    TL_Base = TL_To_Ptr (offset);
    base_copy = TL_To_Ptr (offset);

    "base %p (aligned from %p)\n", TL_Base, true_base;

    for (i = 0; i < num_blocks; i++) {
        file_entry = base_copy + (i * BLK_SIZE);
        if (!FRBlks (f, file_entry, i, 1)) {
            "Unable to read content from '%s' file\n\n", program;
            return FALSE;
        }
    }
    FClose (f);
    "loaded blocks\n";

    // read the relocation table and do the relocations..
    for (i = header.load_size; i < header.total_size; i += 4) {
        table_entry = base_copy + i;
        MemCpy (&reloc, table_entry, 4);
        if (reloc >= (header.load_size / 8)) {
           "relocation %d invalid.\n", reloc;
           return FALSE;
        }
        reloc = reloc * 8;
        MemCpy (&tmp, base_copy + reloc, 8);
        tmp += offset;
        MemCpy (base_copy + reloc, &tmp, 8);
    }
    "relocated blocks\n";

    // Zero area occupied by relocations
    // (It's also the BSS area)
    table_entry = base_copy + header.load_size;
    MemSet (table_entry, 0, header.total_size - header.load_size);

    // Heap space:
    TL_Break_Start = header.end_address + offset;
    TL_Break_End = header.total_size + TL_MAX_HEAP_SIZE + offset;

    // Program space
    TL_Program_Start = offset;
    TL_Program_End = offset + header.fini_address;

    // Setup environment.
    // Args are copied from built-in variables argc and argv
    TL_Base[2] = &TL_Syscall_Point;
    TL_Argv = MAlloc ((argc * 8) + 64);
    MemSet (TL_Argv, 0x00, (argc * 8) + 64);
    TL_Argv[0] = "loader";
    TL_Argc = argc;
    for (i = 1; i < argc; i++) {
        TL_Argv[i] = TL_To_Ptr (argv[i]);
    }
    for (i = 0; i < TL_MAX_FDS; i++) {
        TL_FD_Handle[i].base = NULL;
        TL_FD_Handle[i].remaining = 0;
        TL_FD_Handle[i].position = 0;
    }

    "heap start %p.\n", TL_To_Ptr (TL_Break_Start);
    "heap end %p.\n", TL_To_Ptr (TL_Break_End);

    "launching\n";
    TL_Launch;
    "\nreturned from program\n";
    Free (TL_Argv);
    Free (true_base);
    for (i = 0; i < TL_MAX_FDS; i++) {
        if (TL_FD_Handle[i].base != NULL) {
            Free (TL_FD_Handle[i]);
        }
    }
    "memory freed\n";
    return TRUE;
}

U0 TL_Demo ()
{
    TL_Loader ("program", "detective.z5");
}

TL_Demo;

