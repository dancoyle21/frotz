
U64 TL_MAX_HEAP_SIZE = 0x100000;
U64 * TL_Base;

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

U0 TL_Syscall()
{
    "System call\n";
}

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

    TL_Base[2] = &TL_Syscall;
    return TRUE;
}

TL_Loader;
