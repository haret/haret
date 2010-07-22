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
#include "machines.h" // Mach
#include "memcmds.h"


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
void memPhysFill(uint32 paddr, uint32 wcount, uint32 value, int wordsize)
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
 * Writing or clearing bits to memory
 ****************************************************************/

static void
setbitVirt(uint8 *vaddr, uint32 bitnr, uint32 bitval)
{
    TRY_EXCEPTION_HANDLER {
        if (bitval)
        {
          *(uint32*)vaddr |= (1 << bitnr - 1);
        }
        else
        {
          *(uint32*)vaddr &= ~(1 << bitnr - 1);
        }
    } CATCH_EXCEPTION_HANDLER {
        Output(C_ERROR "EXCEPTION while writing bit %d at address %p",
               bitnr, vaddr);
    }
}

static void
setbitPhys(uint32 paddr, uint32 bitnr, uint32 bitval)
{
  uint8 *vaddr = memPhysMap(paddr);
  setbitVirt(vaddr, bitnr, bitval);
}


static void
cmd_setbit(const char *tok, const char *x)
{
    uint32 addr, bitnr, bitval;
    char fill_type = toupper(tok[6]);

    // Get parameters
    if (!get_expression(&x, &addr)
        || !get_expression(&x, &bitnr)
        || !get_expression(&x, &bitval)) {
        ScriptError("Expected <addr> <bitnr> <0|1>");
        return;
    }

    switch (fill_type) {
    case 'V':
        setbitVirt((uint8*)addr, bitnr, bitval);
        break;
    case 'P':
        setbitPhys(addr, bitnr, bitval);
        break;
    }
}


REG_CMD_ALT(0, "SETBITV", cmd_setbit, setbitv, 0)
REG_CMD(0, "SETBITP", cmd_setbit,
        "SETBIT[V|P] <addr> <bitnr> <0|1>\n"
        "  Sets or clears a bit calculated from the given [P]hysical\n"
        "  or [V]irtual base address.")

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

DEF_GETCPR(get_p15r1, p15, 0, c1, c0, 0)
DEF_GETCPR(get_p15r2, p15, 0, c2, c0, 0)
DEF_GETCPR(get_p15r3, p15, 0, c3, c0, 0)
DEF_GETCPR(get_p15r13, p15, 0, c13, c0, 0)

static void
memDumpMMU(const char *tok, const char *args)
{
    uint32 l1only = 0, showall = 1, start = 0, size = 0;
    if (get_expression(&args, &l1only) && get_expression(&args, &start)) {
        size = 1;
        get_expression(&args, &size);
        showall = 0;
    }
    if (l1only != 1)
        l1only = 0;

    Output("----- Virtual address map -----");
    Output(" cp15: r1=%08x r2=%08x r3=%08x r13=%08x\n"
           , get_p15r1(), get_p15r2(), get_p15r3(), get_p15r13());
    Output(
        "Descriptor flags legend:\n"
        "  C: Cacheable    B: Bufferable     D: Domain #\n"
        " AP: Access Permissions (for up to 4 slices):\n"
        "       0: No Access             1: Supervisor mode read/write\n"
        "       2: User mode read        3: User mode read/write");
    if (Mach->arm6mmu) {
        Output(
        "       4: Reserved              5: Supervisor mode read only\n"
        "       6: Supervisor/user read  7: Supervisor/user read\n"
        " XN: No execute   P: ECC enabled   nG: Not-Global\n"
        "  S: Shared       T: 'TEX' code\n"
            );
    }

    uint32 mmu = cpuGetMMU();

    Output("  Virtual | Physical |   Description |  Flags");
    Output("  address | address  |               |");
    Output("----------+----------+---------------+----------------------");

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

            parseL1Entry(mb, l1d, pL1, l1only, showall, start, size);
        }
    } CATCH_EXCEPTION_HANDLER {
        Output(C_ERROR "EXCEPTION CAUGHT AT MEGABYTE %d!", mb);
    }

    if (showall)
        Output("End of virtual address space");
    DoneProgress();
}

//mb=entry no, l1d=l1 descriptor (the entry), pL1 = previous L1
//l1only=don't parse L2, start:size is what to look for (phys)
int parseL1Entry(uint32 mb, uint32 l1d, uint32 pL1,
                    uint32 l1only, uint32 showall, uint32 start, uint32 size ){
    int linesOut = 0; //no. lines outputted
    uint32 vaddr=mb<<20;
    const struct pageinfo *pi = getL1Desc(l1d);
    uint32 paddr = l1d & pi->mask;
    char flagbuf[64];
    pi->flagfunc(flagbuf, l1d & ~pi->mask);
    if (! pi->isMapped) {
        if (showall && (l1d ^ pL1) & MMU_L1_TYPE_MASK){
            Output("%08x  |          | %13s |", vaddr, pi->name);
            linesOut++;
        }
        return linesOut;
    }
    if (! pi->L2MapShift) {
        if (showall
            || RANGES_OVERLAP(start, size, paddr, ~pi->mask + 1)){
            Output("%08x  | %08x | %13s |%s"
           , vaddr, paddr, pi->name, flagbuf);
            linesOut++;
        }
        return linesOut;
    }
    if (showall){
        Output("%08x  |          | %13s |%s", vaddr, pi->name, flagbuf);
        linesOut++;
    }

    if (l1only)
        return linesOut;

    // Walk the 2nd level descriptor table
    uint l2_count = 1 << (20 - pi->L2MapShift);
    uint32 pL2, l2d = 0xffffffff;
    for (uint d = 0; d < l2_count; d++) {
        pL2 = l2d;
        l2d = memPhysRead(paddr + d * 4);
	uint32 l2vaddr = vaddr + (d << pi->L2MapShift);
        const struct pageinfo *pi2 = getL2Desc(l2d);
        uint32 l2paddr = l2d & pi2->mask;
        pi2->flagfunc(flagbuf, l2d & ~pi2->mask);

        if (!pi2->isMapped) {
            if (showall && (l2d ^ pL2) & MMU_L2_TYPE_MASK){
                Output(" %08x |          | %13s |", l2vaddr, pi2->name);
                linesOut++;
            }
            continue;
        }
        if (showall
            || RANGES_OVERLAP(start, size, l2paddr, 1<<pi->L2MapShift)){
            Output(" %08x | %08x | %13s |%s"
           , l2vaddr, l2paddr, pi2->name, flagbuf);
            linesOut++;
        }
    }
    return linesOut;
}
REG_DUMP(0, "MMU", memDumpMMU,
         "MMU [<1|2> [<start> [<size>]]] \n"
         "  Show virtual memory map (4Gb address space). One may give an\n"
         "  optional argument to trim output to the l1 tables. If <start>\n"
         "  and <size> are specified, only those mappings within the\n"
         "  physical address range are shown.")


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
    freePages(data);
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
