
U64 TL_MAX_HEAP_SIZE = 0x100000;
U64 * TL_Base;
U64 * TL_Argv;
U64 * TL_Restore_SP;
U64 * TL_Temporary;
U64 TL_Countdown;
U64 TL_Size;
U64 TL_SP_Start;

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

U0 TL_Restore_Point();

asm {
_TL_GET_SP::
    MOV     RAX,RSP
    RET
}
_extern _TL_GET_SP U64 * TL_Get_SP_1();
_extern _TL_GET_SP U64 TL_Get_SP_2();

U0 TL_Partial_Dump(U8 * filename, U8 * base, U64 size)
{
    CoreHeader cheader;
    U64 num_blocks = (size + BLK_SIZE - 1) / BLK_SIZE;
    U64 i;
    CFile * fd;

    "dumping %p %d -> %d blocks\n", base, size, num_blocks;

    fd = FOpen (filename, "w", 1 + num_blocks);
    if (!fd) {
        "could not be opened\n";
    } else {
        MemSet (&cheader, 0x00, BLK_SIZE);
        cheader.leadin_code_1 = 0x5555555555555555;
        cheader.leadin_code_2 = 0xAAAAAAAAAAAAAAAA;
        cheader.base = base;
        cheader.size = size;
        cheader.blk_size = BLK_SIZE;
        
        FWBlks (fd, &cheader, 0, 1);
        for (i = 1; i <= num_blocks; i++) {
            FWBlks (fd, base, i, 1);
            base = base + BLK_SIZE;
        }
        FClose (fd);
    }
}

U0 TL_Core_Dump()
{
    U64 * sp1 = TL_Get_SP_1;
    U64 sp2 = TL_Get_SP_2;

    TL_Partial_Dump ("core1", TL_Base, TL_Size);
    TL_Partial_Dump ("core2", sp1, TL_SP_Start - sp2);
    TL_Partial_Dump ("core3", &TL_Get_SP_1, 0x20000);
}

U64 TL_Syscall(U64 number, U64 p0, U64 p1, U64 p2, U64 * s1, U64 * s2)
{
    U64 i;
    U64 copy;
    I8 xtmp;
    U8 * buf;

    "core dump\n";
    TL_Core_Dump;
    "restore point... %p %p\n", TL_Restore_SP, TL_Restore_SP[0];
    TL_Restore_Point ();
    if (TL_Countdown == 0) {
        number += 1000;
    } else {
        TL_Countdown --;
    }
    switch (number) {
        case 1:
            // write
            copy = p1;
            MemCpy (&buf, &copy, 8);
            if (p2 > 1000) {
                "Printing too much: %d\n", p2;
                while (1) {}
                TL_Restore_Point ();
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
            "TL_Restore_SP = %p   s1 = %p  s2 = %p\n", TL_Restore_SP, s1, s2;
            TL_Core_Dump;
            while (1) {}
            TL_Restore_Point ();
            return p2;
        default:
            "\n\nsystem call reached, number = %d, p0 = %d, p1 = %d, p2 = %d\n",
                number, p0, p1, p2;
            i = 1000;
            while ((s1 <= s2) && (i > 0)) {
                "stack[%p] = %d\n", s1, s1[0];
                s1 ++;
                i --;
            }
            "go to restore\n";
            while (1) {}
            TL_Restore_Point ();
            return 0;
    }
    return 0;
}

asm {
_TL_LAUNCH::
    PUSH    RBP
    MOV     RBP, RSP

    PUSH_REGS
    MOV     RAX,0xAAAAAAAAAAAAAAAA

    MOV     RCX,RSP
    PUSH    RCX

    MOV     RDI,1
    MOV     I64 [&TL_Restore_SP], RCX
    MOV     RSI,I64 [&TL_Argv]
    MOV     RCX,I64 [&TL_Base]

    MOV     RBX,I64 [&TL_Restore_SP]
    PUSH    RBX

    PUSH    RAX
    PUSH    RAX
    PUSH    RAX
    PUSH    RAX

    CALL    RCX

_TL_SYSCALL_POINT::
    PUSH_REGS
    // TempleOS: push args on stack in right-to-left order
    //           Called function cleans up the stack
    // Linux: args in registers (l->r): RDI, RSI, RDX, RCX
    // Both: returned value in RAX

    MOV     RAX,0xBBBBBBBBBBBBBBBB
    MOV     RBX,0xDDDDDDDDDDDDDDDD
    PUSH    RAX
    PUSH    RBX
    PUSH    RAX
    PUSH    RBX
    PUSH    RAX
    PUSH    RBX
    PUSH    RAX
    PUSH    RBX

    MOV     RAX,I64 [&TL_Restore_SP]
    PUSH    RAX
    MOV     RAX,RSP
    PUSH    RAX

    PUSH    RCX
    PUSH    RDX
    PUSH    RSI
    PUSH    RDI
    CALL    &TL_Syscall

    POP     RBX
    POP     RBX
    POP     RBX
    POP     RBX
    POP     RBX
    POP     RBX
    POP     RBX
    POP     RBX

    MOV     I64 [&TL_Temporary],RAX
    POP_REGS
    MOV     RAX,I64 [&TL_Temporary]
    RET

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
    TL_Countdown = 10;
    TL_Size = header.total_size;
    TL_SP_Start = TL_Get_SP_2();

    "launching\n";
    TL_Launch;
    "\nreturned from program\n";
    Free (TL_Argv);
    Free (TL_Base);
    "memory freed\n";
    return TRUE;
}


TL_Loader;
