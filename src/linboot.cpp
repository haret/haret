/*
    Linux loader for Windows CE
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include "haret.h"
#include "xtypes.h"
#define CONFIG_ACCEPT_GPL
#include "setup.h"
#include "memory.h"
#include "util.h"
#include "output.h"
#include "gpio.h"
#include "video.h"
#include "cpu.h"
#include "resource.h"

#pragma warning(4:4509)

// Kernel file name
char *bootKernel = "zimage";
// Initrd file name
char *bootInitrd = "initrd";
// Kernel command line
char *bootCmdline = "root=/dev/ram0 ro console=tty0";
// Milliseconds to sleep for nicer animation :-)
uint32 bootSpeed = 5;
// ARM machine type (see linux/arch/arm/tools/mach-types)
uint32 bootMachineType = 339;

// Our own assembly functions
extern "C" uint32 linux_start (uint32 MachType, uint32 NumPages,
  uint32 KernelPA, uint32 PreloaderPA, uint32 TagKernelNumPages);
extern "C" void linux_preloader ();
extern "C" void linux_preloader_end ();

/* Set up kernel parameters. ARM/Linux kernel uses a series of tags,
 * every tag describe some aspect of the machine it is booting on.
 */
static void setup_linux_params (uint8 *tagaddr, uint32 initrd, uint32 initrd_size)
{
  struct tag *tag;

  tag = (struct tag *)tagaddr;

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
    tag->u.initrd.start = initrd;
    tag->u.initrd.size = initrd_size;
    tag = tag_next (tag);
  }

  // now the NULL tag
  tag->hdr.tag = ATAG_NONE;
  tag->hdr.size = 0;
}

/* Loading process:
 * function do_it is loaded onto address ADDR_KERNEL along with parameters
 * (offset=0x100) and kernel image (offset=0x8000). Afterwards DRAMloader
 * is called; it disables MMU and jumps onto ADDR_KERNE. Function do_it
 * then copies kernel image to its proper address (0xA0008000) and calls it.
 * Initrd is loaded onto address INITRD and the address is passed to kernel
 * via ATAG.
 */

// This resets some devices
void ResetDevices ()
{
  // Reset AC97
  memPhysWrite (0x4050000C,0);
  // Reset PCMCIA
  memPhysWrite (0x48000014,0);
  // Set DMAs to Stop state
  for(int i = 0; i < 0x3C; i += 4)
    memPhysWrite (0x40000000,8);
  // Disable DMA interrupts
  memPhysWrite (0x400000F0,0);

  gpioSetDir (28, false);
  gpioSetDir (29, false);
  gpioSetDir (30, false);
  gpioSetDir (31, false);
  gpioSetDir (32, false);

  gpioSetAlt (28, 0);
  gpioSetAlt (29, 0);
  gpioSetAlt (30, 0);
  gpioSetAlt (31, 0);
  gpioSetAlt (32, 0);
}

// Whew... a real Microsoft API function (by number of parameters :)
static bool read_file (FILE *f, uint8 *buff, uint32 size, uint32 totsize,
                       uint32 &totread, videoBitmap &thermored, int dx, int dy)
{
  uint8 *cur = buff;
  uint32 th = thermored.GetHeight ();
  uint sy1 = (totread * th) / totsize;
  while (size)
  {
    int c = size;
    if (c > 16 * 1024)
      c = 16 * 1024;
    c = fread (cur, 1, c, f);
    if (c <= 0)
      return false;

    size -= c;
    cur += c;

    totread += c;
    uint sy2 = (totread * th) / totsize;

    while (sy1 < sy2)
    {
      thermored.DrawLine (dx + THERMOMETER_X, dy + THERMOMETER_Y +
                          th - 1 - sy1, sy1);
      sy1++;
      Sleep (bootSpeed);
    }
  }

  return true;
}

/* LINUX BOOT PROCESS
 *
 * Since we need to load Linux at the beginning of physical RAM, it will
 * overwrite the MMU L1 table which is located in WinCE somewhere around
 * there. Thus we have either to copy the kernel and initrd twice (once
 * to some well-known location and second time to the actual destination
 * address (0xa0008000 for XScale), or to use a clever algorithm (which
 * is used here):
 *
 * A "kernel bundle" is prepared. It consists of the following things:
 * - Several values to be further passed to kernel (such as machine type).
 * - The tag list to be passed to kernel.
 * - The kernel
 * - The initrd
 *
 * Then we proceed as follows: create a one-to-one virtual:physical mapping,
 * then copy to the very end of physical RAM (where there's little chance of
 * overwriting something important) a little pre-loader along with a large
 * table containing a list of physical addresses for every 4k page of the
 * kernel bundle.
 *
 * Now when the pre-loader gets control, it disables MMU and starts copying
 * kernel bundle to desired location (since it knows the physical location).
 * Finally, it passes control to kernel.
 */
void bootLinux ()
{
  char fn [200];

  fnprepare (bootKernel, fn, sizeof (fn) / sizeof (wchar_t));
  FILE *fk = fopen (fn, "rb");
  if (!fk)
  {
    Complain (C_ERROR ("Failed to load kernel %hs"), fn);
    return;
  }

  uint32 i, ksize, isize = 0;

  // Find out kernel image size
  fseek (fk, 0, SEEK_END);
  ksize = ftell (fk);
  fseek (fk, 0, SEEK_SET);

  FILE *fi;
  if (bootInitrd && *bootInitrd)
  {
    fnprepare (bootInitrd, fn, sizeof (fn) / sizeof (wchar_t));
    fi = fopen (fn, "rb");
    if (fi)
    {
      fseek (fi, 0, SEEK_END);
      isize = ftell (fi);
      fseek (fi, 0, SEEK_SET);
    }
  }

  // Load the bitmaps used to display load progress
  videoBitmap logo (IDB_LOGO);
  videoBitmap thermored (IDB_THERMORED);
  videoBitmap thermoblue (IDB_THERMOBLUE);
  videoBitmap eyes (IDB_EYES);

  videoBeginDraw ();

  int dx = (videoW - logo.GetWidth ()) / 2;
  int dy = (videoH - logo.GetHeight ()) / 2;

  logo.Draw (dx, dy);
  thermoblue.Draw (dx + THERMOMETER_X, dy + THERMOMETER_Y);

  // Align kernel and initrd sizes to nearest 4k boundary.
  uint totsize = ksize + isize;
  uint aksize = (ksize + 4095) & ~4095;
  uint aisize = (isize + 4095) & ~4095;

  // Construct kernel bundle (tags + kernel + initrd)
  uint8 *kernel_bundle;
  uint kbsize = 4096 + aksize + aisize;

  // We have to ensure that physical memory allocated kernel is not
  // located too low. If we won't do this it could happen that we'll overlap
  // memory locations when copying from the allocated buffer to destination
  // physical address.
  uint alloc_tries = 10;
  uint kernel_addr = memPhysAddr + 0x8000;
  Output (L"Physical kernel address: %08x", kernel_addr);

  GarbageCollector gc;

  for (;;)
  {
    uint mem = (uint)malloc (kbsize + 4095);
    if (!mem)
    {
      Output (L"FATAL: Not enough memory for kernel bundle!");
      gc.FreeAll ();
      return;
    }

    // Kernel bundle should start at the beginning of page
    kernel_bundle = (uint8 *)((mem + 4095) & ~0xfff);
    // Since we copy the kernel to the beginning of physical memory page
    // by page it will be enough if we ensure that every allocated page has
    // physical address larger than its final physical address.
    bool ok = true;
    for (i = 0; i <= (kbsize >> 12); i++)
      if (memVirtToPhys ((uint32)kernel_bundle + (i << 12)) < kernel_addr + (i << 12))
      {
        ok = false;
        break;
      }

    if (ok)
      break;

    gc.Collect ((void *)mem);
    Output (L"WARNING: page %d has addr %08x, target addr %08x, retrying",
            i, memVirtToPhys ((uint32)kernel_bundle + (i << 12)),
            kernel_addr + (i << 12));

    if (!--alloc_tries)
    {
      Output (L"FATAL: Cannot allocate kernel bundle in high memory!");
      gc.FreeAll ();
      return;
    }
  }

  gc.FreeAll ();

  uint8 *taglist = kernel_bundle;
  uint8 *kernel = taglist + 4096;
  uint8 *initrd = kernel + aksize;

  // Now read the kernel and initrd
  uint totread = 0;

  if (!read_file (fk, kernel, ksize, totsize, totread, thermored, dx, dy))
  {
    Complain (C_ERROR ("Error reading kernel `%hs'"), bootKernel);
errexit:
    free (kernel);
    return;
  }
  fclose (fk);

  if (isize)
  {
    if (!read_file (fi, initrd, isize, totsize, totread, thermored, dx, dy))
    {
      Complain (C_ERROR ("Error reading initrd `%hs'"), bootInitrd);
      goto errexit;
    }
    fclose (fi);
  }

  // Now construct our "page table" which will work in absense of MMU
  uint npages = (4096 + aksize + aisize) >> 12;
  uint preloader_size = (0x100 + npages * 4 + 0xfff) & ~0xfff;

  uint8 *preloader;
  uint32 preloaderPA;
  uint32 *ptable;

  // We need the preloader to be contiguous in physical memory.
  // It's not so big (usually one or two 4K pages), however we may
  // need to allocate it a couple of times till we get it as we need it...
  // Also, naturally, preloader cannot be located in the memory where
  // kernel will be copied.
  alloc_tries = 10;
  for (;;)
  {
    uint32 mem = (uint32)malloc (preloader_size + 4095);
    if (!mem)
    {
      Output (L"FATAL: Not enough memory for preloader!");
      gc.FreeAll ();
      return;
    }

    preloader = (uint8 *)((mem + 0xfff) & ~0xfff);
    preloaderPA = memVirtToPhys ((uint32)preloader);

    bool ok = true;
    uint32 minaddr = memPhysAddr + 0x7000 + kbsize;
    for (i = 1; i < (preloader_size >> 12); i++)
    {
      uint32 pa = memVirtToPhys ((uint32)preloader + (i << 12));
      if ((pa != (preloaderPA + (i << 12))) || (pa < minaddr))
      {
        ok = false;
        break;
      }
    }

    if (ok)
      break;

    gc.Collect ((void *)mem);

    if (!--alloc_tries)
    {
      Output (L"FATAL: Cannot allocate a contiguous physical memory area!");
      gc.FreeAll ();
      return;
    }
  }

  gc.FreeAll ();

  Output (L"Preloader physical/virtual address: %08x", preloaderPA);

  memcpy (preloader, &linux_preloader,
          (uint)&linux_preloader_end - (uint)&linux_preloader);
  ptable = (uint32 *)(preloader + 0x100);

  for (i = 0; i < npages; i++)
    ptable [i] = memVirtToPhys ((uint32)kernel_bundle + (i << 12));

  // Recommended kernel placement = RAM start + 32K
  // Initrd will be put at the address of kernel + 4Mb
  // (let's hope uncompressed kernel never happens to be larger than that).
  uint32 initrd_phys_addr = memPhysAddr + 0x8000 + 0x400000;
  if (isize)
    Output (L"Physical initrd address: %08x", initrd_phys_addr);

  setup_linux_params (taglist, initrd_phys_addr, isize);

  Output (L"Goodbye cruel world ...");
  Sleep (500);

  uint32 old_icmr;
  uint32 *icmr = (uint32 *)memPhysMap (ICMR);
  uint32 *mmu = (uint32 *)memPhysMap (cpuGetMMU ());

  eyes.Draw (dx + PENGUIN_EYES_X, dy + PENGUIN_EYES_Y);

  /* Set thread priority to maximum */
  SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_TIME_CRITICAL);
  /* Disable multitasking (heh) */
  CeSetThreadQuantum (GetCurrentThread (), 0);
  /* Allow current process to access any memory domains */
  SetProcPermissions (0xffffffff);
  /* Go into kernel mode (well, wince is always in system mode...) */
  SetKMode (TRUE);

  cli ();
  old_icmr = *icmr;
  *icmr = 0;

  __try
  {
    //ResetDevices ();

    //cpuSetDACR (0xffffffff);

    // Create the virtual->physical 1:1 mapping entry in
    // 1st level descriptor table. These addresses are hopefully
    // unused by WindowsCE (or rather unimportant for us now).
    cpuFlushCache ();
    mmu [preloaderPA >> 20] = (preloaderPA & MMU_L1_SECTION_MASK) |
        MMU_L1_SECTION | MMU_L1_AP_MASK;

    // Penguinize!
    linux_start (bootMachineType, npages, memPhysAddr, preloaderPA,
                 1 + (aksize >> 12));
  }
  // We should never get here, but if we crash try to recover ...
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    // UnresetDevices???
    *icmr = old_icmr;
    sti ();
    SetKMode (FALSE);
    videoEndDraw ();
    Output (L"Linux boot failed because of a exception!");
  };
}
