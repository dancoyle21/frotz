
U64 TL_MAX_HEAP_SIZE = 0x100000;
U64 * TL_Base;
U64 * TL_Argv;
U64 * TL_Restore_SP;

class ProgramHeader
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

U64 TL_Syscall(U64 number, U64 p0, U64 p1, U64 p2, U64 * s1, U64 * s2)
{
    U64 i = 100;

    "system call reached, number = %d, p0 = %d, p1 = %d, p2 = %d\n",
        number, p0, p1, p2;
    
    while ((s1 <= s2) && (i > 0)) {
        "stack[%p] = %d\n", s1, s1[0];
        s1 ++;
        i --;
    }
    "s1 = %p s2 = %p i = %d\n", s1, s2, i;
    return 0;
}

asm {
_TL_LAUNCH::
    PUSH    RBP
    MOV     RBP, RSP

    PUSH_REGS

    MOV     RCX,RSP
    MOV     RDI,1
    MOV     I64 [&TL_Restore_SP], RCX
    MOV     RSI,I64 [&TL_Argv]
    MOV     RCX,I64 [&TL_Base]
    CALL    RCX

_TL_SYSCALL_POINT::
    PUSH_REGS
    // TempleOS: push args on stack in right-to-left order
    //           Called function cleans up the stack
    // Linux: args in registers (l->r): RDI, RSI, RDX, RCX
    // Both: returned value in RAX

    MOV     RAX,I64 [&TL_Restore_SP]
    PUSH    RAX
    MOV     RAX,RSP
    PUSH    RAX

    PUSH    RCX
    PUSH    RDX
    PUSH    RSI
    PUSH    RDI
    CALL    &TL_Syscall
    POP_REGS

_TL_RESTORE_POINT::
    MOV     RCX,I64 [&TL_Restore_SP]
    MOV     RSP,RCX

    POP_REGS

    POP     RBP
    RET
}
_extern _TL_LAUNCH U0 TL_Launch();
_extern _TL_RESTORE_POINT U0 TL_Restore_Point();
_extern _TL_SYSCALL_POINT U0 TL_Syscall_Point();

Bool TL_Loader()
{
    ProgramHeader header;
    CFile *f;
    U64 num_blocks;
    U64 offset;
    U8 * base_copy;
    U8 * table_entry;
    U8 * file_entry;
    U64 i, tmp;
    U32 reloc;

    // read first block
    f = FOpen ("program", "r");
    if (!f) {
        "Unable to open 'program' file\n\n";
        return FALSE;
    }
    if (!FRBlks (f, &header, 0, 1)) {
        "Unable to read header from 'program' file\n\n";
        return FALSE;
    }
    "program header:\n";
    "fini_address %d.\n", header.fini_address;
    "end_address %d.\n", header.end_address;
    "load_size %d.\n", header.load_size;
    "total_size %d.\n", header.total_size;


    num_blocks = (header.total_size + BLK_SIZE - 1) / BLK_SIZE;

    "number of blocks %d.\n", num_blocks;

    // allocate
    TL_Base = MAlloc (TL_MAX_HEAP_SIZE + header.total_size);
    "base %p.\n", TL_Base;

    MemSet (TL_Base, 0xcc, TL_MAX_HEAP_SIZE + header.total_size);

    // Make aliases to TL_Base
    MemCpy (&offset, &TL_Base, 8);
    MemCpy (&base_copy, &TL_Base, 8);

    for (i = 0; i < num_blocks; i++) {
        file_entry = base_copy + (i * BLK_SIZE);
        if (!FRBlks (f, file_entry, i, 1)) {
            "Unable to read content from 'program' file\n\n";
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
        MemCpy ( base_copy + reloc, &tmp, 8);
    }
    "relocated blocks\n";

    TL_Base[2] = &TL_Syscall_Point;
    TL_Argv = MAlloc(32);
    MemSet (TL_Argv, 0x00, 32);
    TL_Argv[0] = "loader";

    "launching\n";
    TL_Launch;
    "\nreturned from program\n";
    Free (TL_Argv);
    Free (TL_Base);
    "memory freed\n";
    return TRUE;
}


TL_Loader;
