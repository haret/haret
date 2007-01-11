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

REG_VAR_STR(0, "KERNEL", bootKernel, "Linux kernel file name")
REG_VAR_STR(0, "INITRD", bootInitrd, "Initial Ram Disk file name")
REG_VAR_STR(0, "CMDLINE", bootCmdline, "Kernel command line")
REG_VAR_INT(0, "MTYPE", bootMachineType
            , "ARM machine type (see linux/arch/arm/tools/mach-types)")

// Color codes useful when writing to framebuffer.
enum {
    COLOR_BLACK   = 0x0000,
    COLOR_WHITE   = 0xFFFF,
    COLOR_RED     = 0xf800,
    COLOR_GREEN   = 0x07e0,
    COLOR_BLUE    = 0x001f,
    COLOR_YELLOW  = COLOR_RED | COLOR_GREEN,
    COLOR_CYAN    = COLOR_GREEN | COLOR_BLUE,
    COLOR_MAGENTA = COLOR_RED | COLOR_BLUE,
};

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
 * code will try to write a status indicator to the video screen to
 * indicate its progress.  This can be used to help diagnose failures
 * during the boot.  A green line is written after disabling
 * interrupts, a magenta line is written after disabling hardware
 * (Mach->hardwareShutdown), a blue line after starting the preloader
 * function, a red line after copying the "linux tags" structure, a
 * cyan line after copying the kernel, a yellow line after copying the
 * initrd (if any), and finally a black line right before jumping to
 * the kernel.  If CRC checking is enabled (via the variable
 * KERNELCRC) then the kernel and CRC are checked between the yellow
 * and black lines - a red line is written if the kernel crc
 * mismatches and a magenta line is written if the initrd crc
 * mismatches.
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

// Mark a function that is used in the C preloader.  Note all
// functions marked this way will be copied to physical ram for the
// preloading and are run with the MMU disabled.  These functions must
// be careful to not call functions that aren't also marked this way.
// They must also not use any global variables.
#define __preload __attribute__ ((__section__ (".text.preload")))

// Maximum number of index pages.
#define MAX_INDEX 5
#define PAGES_PER_INDEX (PAGE_SIZE / sizeof(uint32))

// Data Shared between normal haret code and C preload code.
struct preloadData {
    uint32 machtype;
    uint32 videoRam;
    uint32 startRam;

    char *tags;
    uint32 kernelSize;
    uint32 initrdSize;
    const char **indexPages[MAX_INDEX];

    // Optional CRC check
    uint32 doCRC;
    uint32 kernelCRC, initrdCRC;
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
static void __preload
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

// Draw a line directly onto the frame buffer.
static void __preload
drawLine(uint32 *pvideoRam, uint16 color)
{
    enum { LINELENGTH = 2500 };
    uint32 videoRam = *pvideoRam;
    if (! videoRam)
        return;
    uint16 *pix = (uint16*)videoRam;
    for (int i = 0; i < LINELENGTH; i++)
        pix[32768+i] = color;
    *pvideoRam += LINELENGTH * sizeof(uint16);
}

// Code to launch kernel.
static void __preload
preloader(struct preloadData *data)
{
    drawLine(&data->videoRam, COLOR_BLUE);

    // Copy tags to beginning of ram.
    char *destTags = (char *)data->startRam + PHYSOFFSET_TAGS;
    do_copy(destTags, data->tags, TAGSIZE);

    drawLine(&data->videoRam, COLOR_RED);

    // Copy kernel image
    char *destKernel = (char *)data->startRam + PHYSOFFSET_KERNEL;
    int kernelCount = PAGE_ALIGN(data->kernelSize) / PAGE_SIZE;
    do_copyPages((char *)destKernel, data->indexPages, 0, kernelCount);

    drawLine(&data->videoRam, COLOR_CYAN);

    // Copy initrd (if applicable)
    char *destInitrd = (char *)data->startRam + PHYSOFFSET_INITRD;
    int initrdCount = PAGE_ALIGN(data->initrdSize) / PAGE_SIZE;
    do_copyPages(destInitrd, data->indexPages, kernelCount, initrdCount);

    drawLine(&data->videoRam, COLOR_YELLOW);

    // Do CRC check (if enabled).
    if (data->doCRC) {
        uint32 crc = crc32_be(0, destKernel, data->kernelSize);
        crc = crc32_be_finish(crc, data->kernelSize);
        if (crc != data->kernelCRC)
            drawLine(&data->videoRam, COLOR_RED);
        if (data->initrdSize) {
            crc = crc32_be(0, destInitrd, data->initrdSize);
            crc = crc32_be_finish(crc, data->initrdSize);
            if (crc != data->initrdCRC)
                drawLine(&data->videoRam, COLOR_MAGENTA);
        }
    }

    drawLine(&data->videoRam, COLOR_BLACK);

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
    Output("Boot FB feedback: %d", Mach->fbDuringBoot);

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
    pd->videoRam = 0;
    bm->pd = pd;

    if (Mach->fbDuringBoot) {
        pd->videoRam = vidGetVRAM();
	unsigned int endKernelStuff = pd->startRam + PHYSOFFSET_INITRD + pd->initrdSize + PAGE_SIZE;
        if (pd->videoRam >= pd->startRam && pd->videoRam < endKernelStuff) {
            Output("Boot FB feedback requested, but FB overlaps with kernel structures - feedback disabled");
            pd->videoRam = 0;
        } else {
            Output("Video buffer at phys=%08x", pd->videoRam);
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
    void mmu_trampoline(uint32 phys, uint8 *mmu, uint32 code);
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
launchKernel(uint32 physExec)
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

    // Lookup framebuffer address (if in use).
    uint32 vidRam = 0;
    if (Mach->fbDuringBoot) {
        vidRam = (uint32)vidGetVirtVRAM();
        Output("Video buffer at virt=%08x", vidRam);
    }

    // Call per-arch setup.
    int ret = Mach->preHardwareShutdown();
    if (ret) {
        Output(C_ERROR "Setup for machine shutdown failed");
        return;
    }

    Screen("Go Go Go...");

    // Disable interrupts
    take_control();

    drawLine(&vidRam, COLOR_GREEN);

    // Call per-arch boot prep function.
    Mach->hardwareShutdown();

    drawLine(&vidRam, COLOR_MAGENTA);

    // Disable MMU and launch linux.
    mmu_trampoline(physAddrTram, virtAddrMmu, physExec);

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
    cpuFlushCache();
    resume[0] = 0xe51ff004; // ldr pc, [pc, #-4]
    resume[1] = physExec;
    return_control();

    // Wait for user to suspend/resume
    Screen("Ready to boot.  Please suspend/resume");
    Sleep(300 * 1000);

    // Cleanup (if boot failed somehow).
    Output("Timeout. Restoring original resume vector");
    take_control();
    cpuFlushCache();
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
        launchKernel(bm->physExec);

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
