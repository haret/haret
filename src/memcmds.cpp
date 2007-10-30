// Script commands to interact with memory areas.
//
//  Copyright (C) 2003 Andrew Zabolotny
//
//  For conditions of use see file COPYING

#include <ctype.h> // toupper
#include <stdio.h> // FILE

#include "output.h" // Output
#include "memory.h" // memPhysMap
#include "script.h" // ScriptError
#include "cpu.h" // DEF_GETCPR
#include "exceptions.h" // TRY_EXCEPTION_HANDLER
#include "resource.h" // DLG_PROGRESS


/****************************************************************
 * Reading values from memory
 ****************************************************************/

static uchar dump_char (uchar c)
{
  if ((c < 32) || (c >= 127))
    return '.';
  else
    return c;
}

// Read from virtual memory with exception protection
static uint32 memRead(uint8 *vaddr, int wordsize)
{
    uint32 rv = 0;
    TRY_EXCEPTION_HANDLER {
        switch (wordsize) {
        case MO_SIZE8:
            rv = *(uint8*)vaddr;
            break;
        case MO_SIZE16:
            rv = *(uint16*)vaddr;
            break;
        default:
            rv = *(uint32*)vaddr;
            break;
        }
    } CATCH_EXCEPTION_HANDLER {
        Output(C_ERROR "EXCEPTION while reading from address %p", vaddr);
    }
    return rv;
}

static bool alignMemAddr(uint32 *addrLoc)
{
    if (*addrLoc & 3) {
	ScriptError("Unaligned memory address, rounding up");
	*addrLoc &= ~3;
	return true;
    }
    return false;
}

// Dump a portion of memory to file
static void
memDump(uint8 *vaddr, uint32 size, uint32 base = (uint32)-1)
{
    uint32 offs = 0;
    char chrdump[17];
    chrdump[16] = 0;

    if (base == (uint32)-1)
        base = (uint32)vaddr;

    while (offs < size) {
        if ((offs & 15) == 0) {
            if (offs)
                Output(" | %s", chrdump);
            Output("%08x |\t", base + offs);
        }

        uint32 d = memRead(vaddr + offs, MO_SIZE32);
        Output(" %08x\t", d);

        chrdump[(offs & 15) + 0] = dump_char((d      ) & 0xff);
        chrdump[(offs & 15) + 1] = dump_char((d >>  8) & 0xff);
        chrdump[(offs & 15) + 2] = dump_char((d >> 16) & 0xff);
        chrdump[(offs & 15) + 3] = dump_char((d >> 24) & 0xff);

        offs += 4;
    }

    while (offs & 15) {
        Output("         \t");
        chrdump[offs & 15] = 0;
        offs += 4;
    }

    Output(" | %s", chrdump);
}

// Dump a portion of physical memory to file
static void memPhysDump(uint32 paddr, uint32 size)
{
    while (size) {
        uint8 *vaddr = memPhysMap(paddr);
        uint32 bytes = PHYS_CACHE_SIZE - (PHYS_CACHE_MASK & (uint32)vaddr);
        if (bytes > size)
            bytes = size;
        memDump(vaddr, bytes, paddr);
        size -= bytes;
        paddr += bytes;
    }
}

static void
cmd_memaccess(const char *tok, const char *args)
{
    bool virt = toupper(tok[0]) == 'V';
    uint32 addr, size;
    if (!get_expression(&args, &addr) || !get_expression(&args, &size)) {
        ScriptError("Expected <addr> <size>");
        return;
    }

    alignMemAddr(&addr);

    if (virt)
        memDump((uint8 *)addr, size);
    else
        memPhysDump(addr, size);
}
REG_CMD_ALT(0, "VD|UMP", cmd_memaccess, vdump, 0)
REG_CMD(0, "PD|UMP", cmd_memaccess,
        "[V|P]DUMP <addr> <size>\n"
        "  Dump an area of memory in hexadecimal/char format from\n"
        "  given [V]irtual or [P]hysical address.")


/****************************************************************
 * Writing values to memory
 ****************************************************************/

// Fill given number of words in virtual memory with given value
static void memFill(uint8 *vaddr, uint32 wcount, uint32 value, int wordsize)
{
    TRY_EXCEPTION_HANDLER {
        switch (wordsize) {
        case MO_SIZE8:
            while (wcount--) {
                *(uint8*)vaddr = value;
                vaddr += 1;
            }
            break;
        case MO_SIZE16:
            while (wcount--) {
                *(uint16*)vaddr = value;
                vaddr += 2;
            }
            break;
        default:
            while (wcount--) {
                *(uint32*)vaddr = value;
                vaddr += 4;
            }
            break;
        }
    } CATCH_EXCEPTION_HANDLER {
        Output(C_ERROR "EXCEPTION while writing %08x to address %p",
               value, vaddr);
    }
}

// Fill given number of words in physical memory with given value
static void memPhysFill(uint32 paddr, uint32 wcount, uint32 value, int wordsize)
{
  while (wcount)
  {
    uint8 *vaddr = memPhysMap (paddr);
    // We are guaranteed to have 32K ahead
    uint32 words = (32 * 1024) >> wordsize;
    if (words > wcount)
      words = wcount;
    memFill (vaddr, words, value, wordsize);
    wcount -= words;
    paddr += words << wordsize;
  }
}

static void
cmd_memfill(const char *tok, const char *x)
{
    uint32 addr, size, value;
    char fill_type = toupper(tok[0]);
    char fill_size = toupper(tok[2]);
    int wordsize = MO_SIZE32;

    if (!get_expression(&x, &addr)
        || !get_expression(&x, &size)
        || !get_expression(&x, &value)) {
        ScriptError("Expected <addr> <size> <value>");
        return;
    }

    switch (fill_size) {
    case 'B':
        wordsize = MO_SIZE8;
        break;
    case 'H':
        wordsize = MO_SIZE16;
        break;
    }

    switch (fill_type) {
    case 'V':
        memFill((uint8*)addr, size, value, wordsize);
        break;
    case 'P':
        memPhysFill(addr, size, value, wordsize);
        break;
    }
}
REG_CMD_ALT(0, "VFB", cmd_memfill, vfb, 0)
REG_CMD_ALT(0, "VFH", cmd_memfill, vfh, 0)
REG_CMD_ALT(0, "VFW", cmd_memfill, vfw, 0)
REG_CMD_ALT(0, "PFB", cmd_memfill, pfb, 0)
REG_CMD_ALT(0, "PFH", cmd_memfill, pfh, 0)
REG_CMD(0, "PFW", cmd_memfill,
        "[V|P]F[B|H|W] <addr> <count> <value>\n"
        "  Fill memory at given [V]irtual or [P]hysical address with a value.\n"
        "  The [B]yte/[H]alfword/[W]ord suffixes selects the size of\n"
        "  <value> and in which units the <count> is measured.")


/****************************************************************
 * Dumping memory directly to file
 ****************************************************************/

static bool memWrite (FILE *f, uint32 addr, uint32 size)
{
  while (size)
  {
    uint32 wc, sz = (size > 0x1000) ? 0x1000 : size;
    TRY_EXCEPTION_HANDLER
    {
      wc = fwrite ((void *)addr, 1, sz, f);
    }
    CATCH_EXCEPTION_HANDLER
    {
      wc = 0;
      Output(C_ERROR "Exception caught!");
    }

    if (wc != sz)
    {
      Output(C_ERROR "Short write detected while writing to file");
      fclose (f);
      return false;
    }
    addr += sz;
    size -= sz;
  }
  return true;
}

// Write a portion of physical memory to file
static bool memPhysWriteFile (FILE *f, uint32 addr, uint32 size)
{
  while (size)
  {
    uint8 *vaddr = memPhysMap (addr);
    // We are guaranteed to have 32K ahead
    uint32 sz = size > 0x8000 ? 0x8000 : size;
    if (!memWrite (f, (uint32)vaddr, sz))
      return false;
    size -= sz;
    addr += sz;
  }
  return true;
}

static void
cmd_memtofile(const char *tok, const char *args)
{
    bool virt = toupper (tok [0]) == 'V';
    char rawfn[MAX_CMDLEN], fn[MAX_CMDLEN];
    if (get_token(&args, rawfn, sizeof(rawfn))) {
        ScriptError("file name expected");
        return;
    }

    uint32 addr, size;
    if (!get_expression(&args, &addr) || !get_expression(&args, &size)) {
        ScriptError("Expected <filename> <address> <size>");
        return;
    }

    fnprepare(rawfn, fn, sizeof(fn));
    FILE *f = fopen(fn, "wb");
    if (!f) {
        Output(C_ERROR "Cannot write file %s", fn);
        return;
    }

    if (virt)
        memWrite(f, addr, size);
    else
        memPhysWriteFile(f, addr, size);

    fclose(f);
}
REG_CMD_ALT(0, "PWF", cmd_memtofile, pwf, 0)
REG_CMD(0, "VWF", cmd_memtofile,
        "[V|P]WF <filename> <addr> <size>\n"
        "  Write a portion of [V]irtual or [P]hysical memory to given file.")


/****************************************************************
 * Dump mmu table
 ****************************************************************/

static inline char *__flags_cb(char *p, uint32 &d)
{
    *p++ = ' ';
    *p++ = (d & MMU_L1_CACHEABLE) ? 'C' : ' ';
    *p++ = (d & MMU_L1_BUFFERABLE) ? 'B' : ' ';
    d &= ~(MMU_L1_CACHEABLE|MMU_L1_BUFFERABLE);
    return p;
}

static inline char *__flags_other(char *p, uint32 d)
{
    d &= ~MMU_L1_TYPE_MASK;
    if (d)
        p += sprintf(p, " ?=%x", d);
    *p = 0;
    return p;
}

static inline char *__flags_ap(char *p, uint32 &d, int shift) {
    *p++ = '0' + ((d>>shift) & 3);
    d &= ~(3<<shift);
    return p;
}

static void __flags_l1(char *p, uint32 d)
{
    uint32 dmn = (d & MMU_L1_DOMAIN_MASK) >> MMU_L1_DOMAIN_SHIFT;
    d &= ~MMU_L1_DOMAIN_MASK;
    p += sprintf(p, " D=%x", dmn);

    if ((d & MMU_L1_TYPE_MASK) == MMU_L1_SECTION) {
        p = __flags_cb(p, d);
        *p++ = ' '; *p++ = 'A'; *p++ = 'P'; *p++ = '=';
        p = __flags_ap(p, d, MMU_L1_AP_SHIFT);
    }

    p = __flags_other(p, d);
}

static void __flags_l2(char *p, uint32 d)
{
    p = __flags_cb(p, d);

    *p++ = ' '; *p++ = 'A'; *p++ = 'P'; *p++ = '=';
    int j = ((d & MMU_L2_TYPE_MASK) < MMU_L2_TINYPAGE) ? 4 : 1;
    for (int i = 0; i < j; i++)
        p = __flags_ap(p, d, MMU_L2_AP0_SHIFT + 2*i);

    p = __flags_other(p, d);
}

DEF_GETCPR(get_p15r1, p15, 0, c1, c0, 0)
DEF_GETCPR(get_p15r2, p15, 0, c2, c0, 0)
DEF_GETCPR(get_p15r3, p15, 0, c3, c0, 0)
DEF_GETCPR(get_p15r13, p15, 0, c13, c0, 0)

struct pageinfo {
    const char name[12];
    uint32 mask;
    uint32 l2_vaddr_shift;
};

static const struct pageinfo L1PageInfo[] = {
    { "UNMAPPED"},
    { "Coarse", MMU_L1_COARSE_MASK, 12 },
    { "1MB section", MMU_L1_SECTION_MASK },
    { "Fine", MMU_L1_FINE_MASK, 10 },
};

static const struct pageinfo L2PageInfo[] = {
    { "UNMAPPED"},
    { "Large (64K)", MMU_L2_LARGE_MASK },
    { "Small (4K)", MMU_L2_SMALL_MASK },
    { "Tiny (1K)", MMU_L2_TINY_MASK },
};

static void
memDumpMMU(const char *tok, const char *args)
{
    uint32 l1only = 0;
    get_expression(&args, &l1only);

    Output("----- Virtual address map -----");
    Output(" cp15: r1=%08x r2=%08x r3=%08x r13=%08x\n"
           , get_p15r1(), get_p15r2(), get_p15r3(), get_p15r13());
    Output("Descriptor flags legend:\n"
           " C: Cacheable\n"
           " B: Bufferable\n"
           " 0..3: Access Permissions (for up to 4 slices):\n"
           "       0: Supervisor mode Read\n"
           "       1: Supervisor mode Read/Write\n"
           "       2: User mode Read\n"
           "       3: User mode Read/Write\n");

    uint32 mmu = cpuGetMMU();

    Output("  Virtual | Physical | Description |  Flags");
    Output("  address | address  |             |");
    Output("----------+----------+-------------+------------------------");

    // Walk down the 1st level descriptor table
    InitProgress(DLG_PROGRESS, 0x1000);
    uint mb = 0;
    TRY_EXCEPTION_HANDLER {
        uint32 pL1, l1d = 0xffffffff;
        for (mb = 0; mb < 0x1000; mb++) {
            SetProgress(mb);
            pL1 = l1d;

            // Read 1st level descriptor
            l1d = memPhysRead(mmu + mb * 4);
            uint32 vaddr=mb<<20;
            int type = l1d & MMU_L1_TYPE_MASK;
            const struct pageinfo *pi = &L1PageInfo[type];
            uint32 paddr = l1d & pi->mask;
            char flagbuf[64];
            __flags_l1(flagbuf, l1d & ~pi->mask);
            switch (type) {
            case MMU_L1_UNMAPPED:
                if ((l1d ^ pL1) & MMU_L1_TYPE_MASK)
                    Output("%08x  |          | %11s |", vaddr, pi->name);
                continue;
            case MMU_L1_SECTION:
                Output("%08x  | %08x | %11s |%s"
                       , vaddr, paddr, pi->name, flagbuf);
                continue;
            }
            Output("%08x  |          | %11s |%s", vaddr, pi->name, flagbuf);

            if (l1only)
                continue;

            // Walk the 2nd level descriptor table
            uint l2_count = 1 << (20 - pi->l2_vaddr_shift);
            uint32 pL2, l2d = 0xffffffff;
            for (uint d = 0; d < l2_count; d++) {
                pL2 = l2d;
                l2d = memPhysRead(paddr + d * 4);
                uint32 l2vaddr = vaddr + (d << pi->l2_vaddr_shift);
                uint32 l2type = l2d & MMU_L2_TYPE_MASK;
                const struct pageinfo *pi2 = &L2PageInfo[l2type];
                uint32 l2paddr = l2d & pi2->mask;
                __flags_l2(flagbuf, l2d & ~pi2->mask);

                if (l2type == MMU_L2_UNMAPPED) {
                    if ((l2d ^ pL2) & MMU_L2_TYPE_MASK)
                        Output(" %08x |          | %11s |", l2vaddr, pi2->name);
                    continue;
                }
                Output(" %08x | %08x | %11s |%s"
                       , l2vaddr, l2paddr, pi2->name, flagbuf);
            }
        }
    } CATCH_EXCEPTION_HANDLER {
        Output(C_ERROR "EXCEPTION CAUGHT AT MEGABYTE %d!", mb);
    }

    Output(" ffffffff |          |             | End of virtual address space");
    DoneProgress();
}
REG_DUMP(0, "MMU", memDumpMMU,
         "MMU [1]\n"
         "  Show virtual memory map (4Gb address space). One may give an\n"
         "  optional argument to trim output to the l1 tables.")


/****************************************************************
 * Memory location tests
 ****************************************************************/

static void
allocTest(const char *tok, const char *args)
{
    uint32 count;
    if (!get_expression(&args, &count)) {
        ScriptError("Expected <count>");
        return;
    }

    struct pageAddrs pages[count];
    void *data = allocPages(pages, count);
    if (!data)
        return;

    Output("pg#: <virt>   <phys>");
    for (uint i=0; i<count; i++)
        Output("%03d: %08x %08x", i, (uint32)pages[i].virtLoc, pages[i].physLoc);
    freePages(data, count);
}
REG_CMD(0, "ALLOCTEST", allocTest,
        "ALLOCTEST <count>\n"
        "  Allocate <count> number of pages and print the pages'\n"
        "  virtual and physical addresses.")


/****************************************************************
 * Memory access variables
 ****************************************************************/

static uint32 memVar(int phys, int wordsize
                     , bool setval, uint32 *args, uint32 val)
{
    uint8 *vaddr;
    if (phys)
        vaddr = memPhysMap(args[0]);
    else
        vaddr = (uint8*)args[0];
    if (setval) {
        memFill(vaddr, 1, val, wordsize);
        return 0;
    }
    return memRead(vaddr, wordsize);
}

static uint32 memScrVMB(bool setval, uint32 *args, uint32 val)
{
    return memVar(0, MO_SIZE8, setval, args, val);
}
REG_VAR_RWFUNC(0, "VMB", memScrVMB, 1, "Virtual Memory Byte access")

static uint32 memScrVMH(bool setval, uint32 *args, uint32 val)
{
    return memVar(0, MO_SIZE16, setval, args, val);
}
REG_VAR_RWFUNC(0, "VMH", memScrVMH, 1, "Virtual Memory Halfword access")

static uint32 memScrVMW(bool setval, uint32 *args, uint32 val)
{
    return memVar(0, MO_SIZE32, setval, args, val);
}
REG_VAR_RWFUNC(0, "VMW", memScrVMW, 1, "Virtual Memory Word access")

static uint32 memScrPMB(bool setval, uint32 *args, uint32 val)
{
    return memVar(1, MO_SIZE8, setval, args, val);
}
REG_VAR_RWFUNC(0, "PMB", memScrPMB, 1, "Physical Memory Byte access")

static uint32 memScrPMH (bool setval, uint32 *args, uint32 val)
{
    return memVar(1, MO_SIZE16, setval, args, val);
}
REG_VAR_RWFUNC(0, "PMH", memScrPMH, 1, "Physical Memory Halfword access")

static uint32 memScrPMW (bool setval, uint32 *args, uint32 val)
{
    return memVar(1, MO_SIZE32, setval, args, val);
}
REG_VAR_RWFUNC(0, "PMW", memScrPMW, 1, "Physical Memory Word access")

static uint32
var_p2v(bool setval, uint32 *args, uint32 val)
{
    return (uint32)memPhysMap(args[0]);
}
REG_VAR_ROFUNC(0, "P2V", var_p2v, 1
               , "Physical To Virtual address translation")
static uint32
var_v2p(bool setval, uint32 *args, uint32 val)
{
    return (uint32)memVirtToPhys(args[0]);
}
REG_VAR_ROFUNC(0, "V2P", var_v2p, 1
               , "Virtual To Physical address translation")
