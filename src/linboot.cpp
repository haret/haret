/*
 * Linux loader for Windows CE
 *
 * (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
 * Copyright (C) 2003 Andrew Zabolotny
 *
 * This file may be distributed under the terms of the GNU GPL license.
 */

#include <windows.h> // Sleep
#include <stdio.h> // FILE, fopen, fseek, ftell
#include <ctype.h> // toupper

#define CONFIG_ACCEPT_GPL
#include "setup.h"

#include "xtypes.h"
#include "script.h" // REG_CMD
#include "memory.h" // memPhysMap, memPhysAddr, memPhysSize
#include "output.h" // Output, Screen, fnprepare
#include "cpu.h" // take_control, return_control, touchAppPages
#include "video.h" // vidGetVRAM
#include "machines.h" // Mach
#include "fbwrite.h" // fb_puts
#include "linboot.h"
#include "resource.h"

// Kernel file name
static char *bootKernel = "zimage";
// Initrd file name
static char *bootInitrd = "initrd";
// Kernel command line
static char *bootCmdline = "root=/dev/ram0 ro console=tty0";
// ARM machine type (see linux/arch/arm/tools/mach-types)
static uint32 bootMachineType = 0;
// Enable framebuffer writes during bootup.
static uint32 FBDuringBoot = 1;

REG_VAR_STR(0, "KERNEL", bootKernel, "Linux kernel file name")
REG_VAR_STR(0, "INITRD", bootInitrd, "Initial Ram Disk file name")
REG_VAR_STR(0, "CMDLINE", bootCmdline, "Kernel command line")
REG_VAR_INT(0, "MTYPE", bootMachineType
            , "ARM machine type (see linux/arch/arm/tools/mach-types)")
REG_VAR_INT(0, "FBDURINGBOOT", FBDuringBoot
            , "Enable/disable writing status lines to screen during boot")

/*
 * Theory of operation:
 *
 * This code is tasked with loading the linux kernel (and an optional
 * initrd) into memory and then jumping to that kernel so that linux
 * can start.  In order to jump to the kernel, all hardware must be
 * disabled (see Mach->hardwareShutdown), the Memory Management Unit
 * (MMU) must be off (see mmu_trampoline in asmstuff.S), and the
 * kernel must be allocated in physically continous ram at a certain
 * position in memory.  Note that it is difficult to allocate
 * physically continuous ram at preset locations while CE is still
 * running (because some other program or the OS might already be
 * using those pages).  To account for this, the code will disable the
 * hardware/mmu, jump to a "preloader" function which will copy the
 * kernel from any arbitrary memory location to the necessary preset
 * areas of memory, and then jump to the kernel.
 *
 * For the above to work, it must load the kernel image into memory
 * while CE is still running.  As a result of this, the kernel is
 * loaded into "virtual memory".  However the preloader is run while
 * the MMU is off and thus sees "physical memory".  A list of physical
 * addresses for each virtual page is maintained so that the preloader
 * can find the proper pages when the mmu is off.  This is complicated
 * because the list itself is built while CE is running (it is
 * allocated in virtual memory) and it can exceed one page in size.
 * To handle this, the system uses a "three level page list" - a list
 * of pointers to pages which contain pointers to pages.  The
 * preloader is passed in a data structure which can not exceed one
 * page (see preloadData).  This structure has a list of pointers to
 * pages (indexPages) that contain pointers to pages of the kernel.
 *
 * Because the preloader and hardware shutdown can be complicated, the
 * code will try to write status messages directly to the framebuffer.
 */


/****************************************************************
 * Linux utility functions
 ****************************************************************/

// Recommended tags placement = RAM start + 256
#define PHYSOFFSET_TAGS   0x100
// Recommended kernel placement = RAM start + 32K
#define PHYSOFFSET_KERNEL 0x8000
// Initrd will be put at the address of kernel + 5MB
#define PHYSOFFSET_INITRD (PHYSOFFSET_KERNEL + 0x500000)
// Maximum size of the tags structure.
#define TAGSIZE (PAGE_SIZE - 0x100)

/* Set up kernel parameters. ARM/Linux kernel uses a series of tags,
 * every tag describe some aspect of the machine it is booting on.
 */
static void
setup_linux_params(char *tagaddr, uint32 phys_initrd_addr, uint32 initrd_size)
{
  struct tag *tag = (struct tag *)tagaddr;

  // Core tag
  tag->hdr.tag = ATAG_CORE;
  tag->hdr.size = tag_size (tag_core);
  tag->u.core.flags = 0;
  tag->u.core.pagesize = 0x00001000;
  tag->u.core.rootdev = 0x0000; // not used, use kernel cmdline for this
  tag = tag_next (tag);

  // now the cmdline tag
  tag->hdr.tag = ATAG_CMDLINE;
  // tag header, zero-terminated string and round size to 32-bit words
  tag->hdr.size = (sizeof (struct tag_header) + strlen (bootCmdline) + 1 + 3) >> 2;
  strcpy (tag->u.cmdline.cmdline, bootCmdline);
  tag = tag_next (tag);

  // now the mem32 tag
  tag->hdr.tag = ATAG_MEM;
  tag->hdr.size = tag_size (tag_mem32);
  tag->u.mem.start = memPhysAddr;
  tag->u.mem.size = memPhysSize;
  tag = tag_next (tag);

  /* and now the initrd tag */
  if (initrd_size)
  {
    tag->hdr.tag = ATAG_INITRD2;
    tag->hdr.size = tag_size (tag_initrd);
    tag->u.initrd.start = phys_initrd_addr;
    tag->u.initrd.size = initrd_size;
    tag = tag_next (tag);
  }

  // now the NULL tag
  tag->hdr.tag = ATAG_NONE;
  tag->hdr.size = 0;
}


/****************************************************************
 * Preloader
 ****************************************************************/

// Maximum number of index pages.
#define MAX_INDEX 5
#define PAGES_PER_INDEX (PAGE_SIZE / sizeof(uint32))

// Data Shared between normal haret code and C preload code.
struct preloadData {
    uint32 machtype;
    uint32 startRam;

    char *tags;
    uint32 kernelSize;
    uint32 initrdSize;
    const char **indexPages[MAX_INDEX];

    // Optional CRC check
    uint32 doCRC;
    uint32 kernelCRC, initrdCRC;

    // Framebuffer info
    fbinfo fbi;
    uint32 physFB, physFonts;
    unsigned char fonts[FONTDATAMAX];
};

// CRC a block of ram (from linux/lib/crc32.c)
#define CRCPOLY_BE 0x04c11db7
static uint32 __preload
crc32_be(uint32 crc, const char *data, uint32 len)
{
    const unsigned char *p = (const unsigned char *)data;
    int i;
    while (len--) {
        crc ^= *p++ << 24;
        for (i = 0; i < 8; i++)
            crc = (crc << 1) ^ ((crc & 0x80000000) ? CRCPOLY_BE : 0);
    }
    return crc;
}
static uint32 __preload
crc32_be_finish(uint32 crc, uint32 len)
{
    for (; len; len >>= 8) {
        unsigned char l = len;
        crc = crc32_be(crc, (char *)&l, 1);
    }
    return ~crc & 0xFFFFFFFF;
}

// Copy memory (need a memcpy with __preload tag).
void __preload
do_copy(char *dest, const char *src, int count)
{
    uint32 *d = (uint32*)dest, *s = (uint32*)src, *e = (uint32*)&src[count];
    while (s < e)
        *d++ = *s++;
}

// Copy a list of pages to a linear area of memory
static void __preload
do_copyPages(char *dest, const char ***pages, int start, int pagecount)
{
    for (int i=start; i<start+pagecount; i++) {
        do_copy(dest, pages[i/PAGES_PER_INDEX][i%PAGES_PER_INDEX], PAGE_SIZE);
        dest += PAGE_SIZE;
    }
}

// Must define strings in __preload section.
#define FB_PUTS(fbi,str) do {                           \
    const char *__msg;                                  \
    asm(".section .text.preload, 1\n"                   \
        "1:      .asciz \"" str "\"\n"                  \
        "        .balign 4\n"                           \
        "        .section .text.preload, 0\n"           \
        "2:      add %0, pc, #( 1b - 2b - 8 )\n"        \
        : "=r" (__msg));                                \
    fb_puts((fbi), __msg);                              \
} while (0)

// Code to launch kernel.
static void __preload
preloader(struct preloadData *data)
{
    data->fbi.fb = (uint16 *)data->physFB;
    data->fbi.fonts = (unsigned char *)data->physFonts;
    FB_PUTS(&data->fbi, "In preloader\\n");

    // Copy tags to beginning of ram.
    char *destTags = (char *)data->startRam + PHYSOFFSET_TAGS;
    do_copy(destTags, data->tags, TAGSIZE);

    FB_PUTS(&data->fbi, "Tags relocated\\n");

    // Copy kernel image
    char *destKernel = (char *)data->startRam + PHYSOFFSET_KERNEL;
    int kernelCount = PAGE_ALIGN(data->kernelSize) / PAGE_SIZE;
    do_copyPages((char *)destKernel, data->indexPages, 0, kernelCount);

    FB_PUTS(&data->fbi, "Kernel relocated\\n");

    // Copy initrd (if applicable)
    char *destInitrd = (char *)data->startRam + PHYSOFFSET_INITRD;
    int initrdCount = PAGE_ALIGN(data->initrdSize) / PAGE_SIZE;
    do_copyPages(destInitrd, data->indexPages, kernelCount, initrdCount);

    FB_PUTS(&data->fbi, "Initrd relocated\\n");

    // Do CRC check (if enabled).
    if (data->doCRC) {
        FB_PUTS(&data->fbi, "Checking crc...");
        uint32 crc = crc32_be(0, destKernel, data->kernelSize);
        crc = crc32_be_finish(crc, data->kernelSize);
        if (crc != data->kernelCRC)
            FB_PUTS(&data->fbi, " KERNEL CRC FAIL FAIL FAIL");
        if (data->initrdSize) {
            crc = crc32_be(0, destInitrd, data->initrdSize);
            crc = crc32_be_finish(crc, data->initrdSize);
            if (crc != data->initrdCRC)
                FB_PUTS(&data->fbi, " INITRD CRC FAIL FAIL FAIL");
        }
        FB_PUTS(&data->fbi, "\\n");
    }

    if ((cpuGetPSR() & 0xc0) != 0xc0)
        FB_PUTS(&data->fbi, "ERROR: IRQS not off\\n");

    FB_PUTS(&data->fbi, "Jumping to Kernel...\\n");

    // Boot
    typedef void (*lin_t)(uint32 zero, uint32 mach, char *tags);
    lin_t startfunc = (lin_t)destKernel;
    startfunc(0, data->machtype, destTags);
}


/****************************************************************
 * Kernel ram allocation and setup
 ****************************************************************/

extern "C" {
    // Asm code
    extern struct stackJumper_s stackJumper;

    // Symbols added by linker.
    extern char preload_start;
    extern char preload_end;
}
#define preload_size (&preload_end - &preload_start)
#define preloadExecOffset ((char *)&preloader - &preload_start)
#define stackJumperOffset ((char *)&stackJumper - &preload_start)
#define stackJumperExecOffset (stackJumperOffset        \
    + (uint32)&((stackJumper_s*)0)->asm_handler)

// Layout of an assembler function that can setup a C stack and entry
// point.  DO NOT CHANGE HERE without also upgrading the assembler
// code.
struct stackJumper_s {
    uint32 stack;
    uint32 data;
    uint32 execCode;
    char asm_handler[1];
};

struct pagedata {
    uint32 physLoc;
    char *virtLoc;
};

static int physPageComp(const void *e1, const void *e2) {
    pagedata *i1 = (pagedata*)e1, *i2 = (pagedata*)e2;
    return (i1->physLoc < i2->physLoc ? -1
            : (i1->physLoc > i2->physLoc ? 1 : 0));
}

// Description of memory alocated by prepForKernel()
struct bootmem {
    char *imagePages[PAGES_PER_INDEX * MAX_INDEX];
    char **kernelPages, **initrdPages;
    uint32 physExec;
    void *allocedRam;
    struct preloadData *pd;
};

// Release resources allocated in prepForKernel.
static void
cleanupBootMem(struct bootmem *bm)
{
    if (!bm)
        return;
    free(bm->allocedRam);
    free(bm);
}

static void *
allocBufferPages(struct pagedata *pages, int pageCount)
{
    int bufSize = pageCount * PAGE_SIZE + PAGE_SIZE;
    void *allocdata = calloc(bufSize, 1);
    if (! allocdata) {
        Output(C_ERROR "Failed to allocate %d pages", pageCount);
        return NULL;
    }

    Output("Allocated load buffer at %p of size %08x", allocdata, bufSize);

    // Find all the physical locations of the pages.
    void *data = (void*)PAGE_ALIGN((uint32)allocdata);
    for (int i = 0; i < pageCount; i++) {
        struct pagedata *pd = &pages[i];
        pd->virtLoc = &((char *)data)[PAGE_SIZE * i];
        *pd->virtLoc = 0xaa;
        pd->physLoc = memVirtToPhys((uint32)pd->virtLoc);
        if (pd->physLoc == (uint32)-1) {
            Output(C_ERROR "Page at %p not mapped", pd->virtLoc);
            free(allocdata);
            return NULL;
        }
    }

    // Sort the pages by physical location.
    qsort(pages, pageCount, sizeof(pages[0]), physPageComp);

    return allocdata;
}

// Allocate memory for a kernel (and possibly initrd), and configure a
// preloader that can launch that kernel.  Note the caller needs to
// copy the kernel and initrd into the pages allocated.
static bootmem *
prepForKernel(uint32 kernelSize, uint32 initrdSize)
{
    // Sanity test.
    if (preload_size > PAGE_SIZE || sizeof(preloadData) > PAGE_SIZE) {
        Output(C_ERROR "Internal error.  Preloader too large");
        return NULL;
    }

    // Determine machine type
    uint32 machType = bootMachineType;
    if (! machType)
        machType = Mach->machType;
    if (! machType) {
        Output(C_ERROR "undefined MTYPE");
        return NULL;
    }
    Output("boot params: RAMADDR=%08x RAMSIZE=%08x MTYPE=%d CMDLINE='%s'"
           , memPhysAddr, memPhysSize, machType, bootCmdline);
    Output("Boot FB feedback: %d", FBDuringBoot);

    // Allocate ram for kernel/initrd
    uint32 kernelCount = PAGE_ALIGN(kernelSize) / PAGE_SIZE;
    int initrdCount = PAGE_ALIGN(initrdSize) / PAGE_SIZE;
    int indexCount = PAGE_ALIGN((initrdCount + kernelCount)
                                * sizeof(char*)) / PAGE_SIZE;
    int totalCount = kernelCount + initrdCount + indexCount + 4;
    if (indexCount > MAX_INDEX) {
        Output(C_ERROR "Image too large (%d+%d) - largest size is %d"
               , kernelSize, initrdSize
               , MAX_INDEX * PAGES_PER_INDEX * PAGE_SIZE);
        return NULL;
    }

    // Allocate data structure.
    struct bootmem *bm = (bootmem*)calloc(sizeof(bootmem), 1);
    if (!bm) {
        Output(C_ERROR "Failed to allocate bootmem struct");
        return NULL;
    }

    struct pagedata pages[PAGES_PER_INDEX * MAX_INDEX + 4];
    bm->allocedRam = allocBufferPages(pages, totalCount);
    if (! bm->allocedRam) {
	cleanupBootMem(bm);
	return NULL;
    }

    Output("Built virtual to physical page mapping");

    struct pagedata *pg_tag = &pages[0];
    struct pagedata *pgs_kernel = &pages[1];
    struct pagedata *pgs_initrd = &pages[kernelCount+1];
    struct pagedata *pgs_index = &pages[initrdCount+kernelCount+1];
    struct pagedata *pg_stack = &pages[totalCount-3];
    struct pagedata *pg_data = &pages[totalCount-2];
    struct pagedata *pg_preload = &pages[totalCount-1];

    Output("Allocated %d pages (tags=%p/%08x kernel=%p/%08x initrd=%p/%08x"
           " index=%p/%08x)"
           , totalCount
           , pg_tag->virtLoc, pg_tag->physLoc
           , pgs_kernel->virtLoc, pgs_kernel->physLoc
           , pgs_initrd->virtLoc, pgs_initrd->physLoc
           , pgs_index->virtLoc, pgs_index->physLoc);

    if (pg_tag->physLoc < memPhysAddr + PHYSOFFSET_TAGS
        || pgs_kernel->physLoc < memPhysAddr + PHYSOFFSET_KERNEL
        || (initrdSize
            && pgs_initrd->physLoc < memPhysAddr + PHYSOFFSET_INITRD)) {
        Output(C_ERROR "Allocated memory will overwrite itself");
        cleanupBootMem(bm);
        return NULL;
    }

    // Setup linux tags.
    setup_linux_params(pg_tag->virtLoc, memPhysAddr + PHYSOFFSET_INITRD
                       , initrdSize);
    Output("Built kernel tags area");

    // Setup kernel/initrd indexes
    for (uint32 i=0; i<kernelCount+initrdCount; i++) {
        uint32 *index = (uint32*)pgs_index[i/PAGES_PER_INDEX].virtLoc;
        index[i % PAGES_PER_INDEX] = pgs_kernel[i].physLoc;
        bm->imagePages[i] = pgs_kernel[i].virtLoc;
    }
    bm->kernelPages = &bm->imagePages[0];
    bm->initrdPages = &bm->imagePages[kernelCount];
    Output("Built page index");

    // Setup preloader data.
    struct preloadData *pd = (struct preloadData *)pg_data->virtLoc;
    pd->machtype = machType;
    pd->tags = (char *)pg_tag->physLoc;
    pd->kernelSize = kernelSize;
    pd->initrdSize = initrdSize;
    for (int i=0; i<indexCount; i++)
        pd->indexPages[i] = (const char **)pgs_index[i].physLoc;
    pd->startRam = memPhysAddr;
    bm->pd = pd;

    if (FBDuringBoot) {
        fb_init(&pd->fbi);
        memcpy(&pd->fonts, fontdata_mini_4x6, sizeof(fontdata_mini_4x6));
        pd->physFonts = pg_data->physLoc + offsetof(struct preloadData, fonts);
        pd->physFB = vidGetVRAM();
        Output("Video Phys FB=%08x Fonts=%08x", pd->physFB, pd->physFonts);
	uint end = pd->startRam + PHYSOFFSET_INITRD + pd->initrdSize + PAGE_SIZE;
        if (pd->physFB >= pd->startRam && pd->physFB < end) {
            Output("Boot FB feedback requested, but FB overlaps with kernel structures - feedback disabled");
            pd->physFB = 0;
        }
    }

    // Setup preloader code.
    memcpy(pg_preload->virtLoc, &preload_start, preload_size);

    stackJumper_s *sj = (stackJumper_s*)&pg_preload->virtLoc[stackJumperOffset];
    sj->stack = pg_stack->physLoc + PAGE_SIZE;
    sj->data = pg_data->physLoc;
    sj->execCode = pg_preload->physLoc + preloadExecOffset;

    bm->physExec = pg_preload->physLoc + stackJumperExecOffset;

    Output("preload=%d@%p/%08x sj=%p stack=%p/%08x data=%p/%08x exec=%08x"
           , preload_size, pg_preload->virtLoc, pg_preload->physLoc
           , sj, pg_stack->virtLoc, pg_stack->physLoc
           , pg_data->virtLoc, pg_data->physLoc, sj->execCode);

    return bm;
}


/****************************************************************
 * Hardware shutdown and trampoline setup
 ****************************************************************/

extern "C" {
    // Assembler code
    void mmu_trampoline(uint32 phys, uint8 *mmu, uint32 code, void (*)(void));
    void mmu_trampoline_end();
}

// Verify the mmu-disabling trampoline.
static uint32
setupTrampoline()
{
    uint32 virtTram = MVAddr((uint32)mmu_trampoline);
    uint32 virtTramEnd = MVAddr((uint32)mmu_trampoline_end);
    if ((virtTram & 0xFFFFF000) != (virtTramEnd & 0xFFFFF000)) {
        Output(C_ERROR "Can't handle trampoline spanning page boundary"
               " (%p %08x %08x)"
               , mmu_trampoline, virtTram, virtTramEnd);
        return 0;
    }
    uint32 physAddrTram = memVirtToPhys(virtTram);
    if (physAddrTram == (uint32)-1) {
        Output(C_ERROR "Trampoline not in physical ram. (virt=%08x)"
               , virtTram);
        return 0;
    }
    uint32 physTramL1 = physAddrTram & 0xFFF00000;
    if (virtTram > physTramL1 && virtTram < (physTramL1 + 0x100000)) {
        Output(C_ERROR "Trampoline physical/virtual addresses overlap.");
        return 0;
    }

    Output("Trampoline setup (tram=%d@%p/%08x/%08x)"
           , virtTramEnd - virtTram, mmu_trampoline, virtTram, physAddrTram);

    return physAddrTram;
}

// Launch a kernel loaded in memory.
static void
launchKernel(struct bootmem *bm)
{
    // Make sure trampoline and "Mach->hardwareShutdown" functions are
    // loaded into memory.
    touchAppPages();

    // Prep the trampoline.
    uint32 physAddrTram = setupTrampoline();
    if (! physAddrTram)
        return;

    // Cache an mmu pointer for the trampoline
    uint8 *virtAddrMmu = memPhysMap(cpuGetMMU());
    Output("MMU setup: mmu=%p/%08x", virtAddrMmu, cpuGetMMU());

    // Call per-arch setup.
    int ret = Mach->preHardwareShutdown();
    if (ret) {
        Output(C_ERROR "Setup for machine shutdown failed");
        return;
    }

    Screen("Go Go Go...");

    // Disable interrupts
    take_control();

    fb_clear(&bm->pd->fbi);
    fb_puts(&bm->pd->fbi, "HaRET boot\nShutting down hardware\n");

    // Call per-arch boot prep function.
    Mach->hardwareShutdown();

    fb_puts(&bm->pd->fbi, "Turning off MMU...\n");

    // Disable MMU and launch linux.
    mmu_trampoline(physAddrTram, virtAddrMmu, bm->physExec, Mach->flushCache);

    // The above should not ever return, but we attempt recovery here.
    return_control();
}


/****************************************************************
 * File reading
 ****************************************************************/

// Open a file on disk.
static FILE *
file_open(const char *name)
{
    Output("Opening file %s", name);
    char fn[200];
    fnprepare(name, fn, sizeof(fn));
    FILE *fk = fopen(fn, "rb");
    if (!fk) {
        Output("Failed to load file %s", fn);
        return NULL;
    }
    return fk;
}

// Find out the size of an open file.
static uint32
get_file_size(FILE *fk)
{
    fseek(fk, 0, SEEK_END);
    uint32 size = ftell(fk);
    fseek(fk, 0, SEEK_SET);
    return size;
}

// Copy data from a file into memory and check for success.
static int
file_read(FILE *f, char **pages, uint32 size)
{
    Output("Reading %d bytes...", size);
    while (size) {
        uint32 s = size < PAGE_SIZE ? size : PAGE_SIZE;
        uint32 ret = fread(*pages, 1, s, f);
        if (ret != s) {
            Output(C_ERROR "Error reading file.  Expected %d got %d", s, ret);
            return -1;
        }
        pages++;
        size -= s;
        AddProgress(s);
    }
    Output("Read complete");
    return 0;
}

// Load a kernel (and possibly initrd) from disk into ram and prep it
// for kernel starting.
static bootmem *
loadDiskKernel()
{
    Output("boot KERNEL=%s INITRD=%s", bootKernel, bootInitrd);

    // Open kernel file
    FILE *kernelFile = file_open(bootKernel);
    if (!kernelFile)
        return NULL;
    uint32 kernelSize = get_file_size(kernelFile);

    // Open initrd file
    FILE *initrdFile = NULL;
    uint32 initrdSize = 0;
    if (bootInitrd && *bootInitrd) {
        initrdFile = file_open(bootInitrd);
        if (initrdFile)
            initrdSize = get_file_size(initrdFile);
    }

    // Obtain ram for the kernel
    int ret;
    struct bootmem *bm = NULL;
    bm = prepForKernel(kernelSize, initrdSize);
    if (!bm)
        goto abort;

    InitProgress(DLG_PROGRESS_BOOT, kernelSize + initrdSize);

    // Load kernel
    ret = file_read(kernelFile, bm->kernelPages, kernelSize);
    if (ret)
        goto abort;
    // Load initrd
    if (initrdFile) {
        ret = file_read(initrdFile, bm->initrdPages, initrdSize);
        if (ret)
            goto abort;
    }

    fclose(kernelFile);
    if (initrdFile)
        fclose(initrdFile);

    DoneProgress();

    return bm;

abort:
    DoneProgress();

    if (initrdFile)
        fclose(initrdFile);
    if (kernelFile)
        fclose(kernelFile);
    cleanupBootMem(bm);
    return NULL;
}


/****************************************************************
 * Resume vector hooking
 ****************************************************************/

static uint32 winceResumeAddr = 0xa0040000;

// Setup a kernel in ram and hook the wince resume vector so that it
// runs on resume.
static void
resumeIntoBoot(uint32 physExec)
{
    // Lookup wince resume address and verify it looks sane.
    uint32 *resume = (uint32*)memPhysMap(winceResumeAddr);
    if (!resume) {
        Output(C_ERROR "Could not map addr %08x", winceResumeAddr);
        return;
    }
    // Check for "b 0x41000 ; 0x0" at the address.
    uint32 old1 = resume[0], old2 = resume[1];
    if (old1 != 0xea0003fe || old2 != 0x0) {
        Output(C_ERROR "Unexpected resume vector. (%08x %08x)", old1, old2);
        return;
    }

    // Overwrite the resume vector.
    take_control();
    Mach->flushCache();
    resume[0] = 0xe51ff004; // ldr pc, [pc, #-4]
    resume[1] = physExec;
    return_control();

    // Wait for user to suspend/resume
    Screen("Ready to boot.  Please suspend/resume");
    Sleep(300 * 1000);

    // Cleanup (if boot failed somehow).
    Output("Timeout. Restoring original resume vector");
    take_control();
    Mach->flushCache();
    resume[0] = old1;
    resume[1] = old2;
    return_control();
}


/****************************************************************
 * Boot code
 ****************************************************************/

static uint32 KernelCRC;
REG_VAR_INT(0, "KERNELCRC", KernelCRC
            , "If set, perform a CRC check on the kernel/initrd.")

// Test the CRC of a set of pages.
static uint32
crc_pages(char **pages, uint32 origsize)
{
    uint32 crc = 0;
    uint32 size = origsize;
    while (size) {
        uint32 s = size < PAGE_SIZE ? size : PAGE_SIZE;
        crc = crc32_be(crc, *pages, s);
        pages++;
        size -= s;
    }
    return crc32_be_finish(crc, origsize);
}

// Boot a kernel loaded into memory via one of two mechanisms.
static void
tryLaunch(struct bootmem *bm, int bootViaResume)
{
    // Setup CRC (if enabled).
    if (KernelCRC) {
        bm->pd->kernelCRC = crc_pages(bm->kernelPages, bm->pd->kernelSize);
        if (bm->pd->initrdSize)
            bm->pd->initrdCRC = crc_pages(bm->initrdPages, bm->pd->initrdSize);
        bm->pd->doCRC = 1;
        Output("CRC test complete.  kernel=%u initrd=%u"
               , bm->pd->kernelCRC, bm->pd->initrdCRC);
    }

    Output("Launching to physical address %08x", bm->physExec);
    if (bootViaResume)
        resumeIntoBoot(bm->physExec);
    else
        launchKernel(bm);

    // Cleanup (if boot failed somehow).
    cleanupBootMem(bm);
}

// Load a kernel from disk, disable hardware, and jump into kernel.
static void
bootLinux(const char *cmd, const char *args)
{
    int bootViaResume = toupper(cmd[0]) == 'R';

    // Load the kernel/initrd/tags/preloader into memory
    struct bootmem *bm = loadDiskKernel();
    if (!bm)
        return;

    // Luanch it.
    tryLaunch(bm, bootViaResume);
}
REG_CMD(0, "BOOT|LINUX", bootLinux,
        "BOOTLINUX\n"
        "  Start booting linux kernel. See HELP VARS for variables affecting boot.")
REG_CMD_ALT(0, "BOOT2", bootLinux, boot2, 0)
REG_CMD_ALT(
    0, "RESUMEINTOBOOT", bootLinux, resumeintoboot,
    "RESUMEINTOBOOT\n"
    "  Overwrite the wince resume vector so that the kernel boots\n"
    "  after suspending/resuming the pda")


/****************************************************************
 * Boot from kernel already in ram
 ****************************************************************/

static void
copy_pages(char **pages, const char *src, uint32 size)
{
    while (size) {
        uint32 s = size < PAGE_SIZE ? size : PAGE_SIZE;
        memcpy(*pages, src, s);
        src += s;
        pages++;
        size -= s;
        AddProgress(s);
    }
}

// Load a kernel already in memory, disable hardware, and jump into
// kernel.
void
bootRamLinux(const char *kernel, uint32 kernelSize
             , const char *initrd, uint32 initrdSize
             , int bootViaResume)
{
    // Obtain ram for the kernel
    struct bootmem *bm = prepForKernel(kernelSize, initrdSize);
    if (!bm)
        return;

    // Copy kernel / initrd.
    InitProgress(DLG_PROGRESS_BOOT, kernelSize + initrdSize);
    copy_pages(bm->kernelPages, kernel, kernelSize);
    copy_pages(bm->initrdPages, initrd, initrdSize);
    DoneProgress();

    // Luanch it.
    tryLaunch(bm, bootViaResume);
}
