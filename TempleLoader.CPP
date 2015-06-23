
U64 TL_MAX_HEAP_SIZE = 0x100000;
U64 * TL_Base;
U64 * TL_Argv;
U64 * TL_Restore_SP;
U64 * TL_Temporary;

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
}
;
class CoreHeader
{
    U64 leadin_code_1;
    U64 leadin_code_2;
    U8 * base;
    U64 size;
    U64 blk_size;
    U8 pad[BLK_SIZE-(5*8)];
};

asm {
    ALIGN   8,OC_NOP
_TL_RESTORE_POINT::

    MOV     RCX,I64 [&TL_Restore_SP]
    MOV     RSP,RCX

    POP_REGS

    POP     RBP
    RET

}
_extern _TL_RESTORE_POINT U0 TL_Restore_Point();

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

U64 TL_Syscall(U64 number, U64 p0, U64 p1, U64 p2)
{
    U64 i;
    U64 copy;
    I8 xtmp;
    U8 * buf;

    switch (number) {
        case 1:
            // write
            copy = p1;
            MemCpy (&buf, &copy, 8);
            if (p2 > 1000) {
                "Printing too much: %d\n", p2;
                return 0;
            }
            "[[";
            for (i = 0; i < p2; i++) {
                xtmp = buf[i];
                if (xtmp == 0x0a) {
                    "\\n";
                } else if (xtmp == 0x0d) {
                    "\\r";
                } else if (xtmp < 0x20) {
                    xtmp = '.';
                    '' xtmp;
                } else {
                    '' xtmp;
                }
            }
            "]]\n";
            return p2;
        case 16:
            return TL_Syscall_Ioctl (p0, p1, p2);
        case 60:
            "exit\n";
            TL_Restore_Point;
            return 0;
        default:
            "\n\nunsupported system call reached, number = %d, p0 = %d, p1 = %d, p2 = %d\n",
                number, p0, p1, p2;
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
    MOV     RDI,1
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

    PUSH    RCX
    PUSH    RDX
    PUSH    RSI
    PUSH    RDI
    CALL    &TL_Syscall

    MOV     I64 [&TL_Temporary],RAX
    POP_REGS
    MOV     RAX,I64 [&TL_Temporary]
    RET
    ALIGN   8,OC_NOP
}

_extern _TL_LAUNCH U0 TL_Launch();
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

    MemSet (TL_Base, 0x0, TL_MAX_HEAP_SIZE + header.total_size);

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
    TL_Argv = MAlloc(64);
    MemSet (TL_Argv, 0x00, 64);
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
