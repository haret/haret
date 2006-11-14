/* This is the HaRET LD linker script.  The purpose of this script is
 * to generate artificial symbols pointing to certain locations within
 * the haret application.
 */
SECTIONS
{
  .text : {
    /* Create two symbols at the start and end of the "irq" code.
     * These symbols allow the "watch irq" code to copy the assembler
     * associated with C functions in irq.cpp to a continuous area of
     * memory.
     */
    irq_start = .;
    *(.text.irq)
    irq_end = .;

    /* As above, create two symbols for the preloader code. */
    preload_start = .;
    *(.text.preload)
    preload_end = .;

    /* Make sure the trampline is aligned to the start of a page. */
    . = ALIGN(4096);
    *(.text.trampoline)
  }
  .rdata : {
    /* Create two symbols around the list of machine init functions. */
    mach_start = .;
    *(.rdata.mach)
    mach_end = .;

    /* List of haret commands */
    commands_start = .;
    *(.rdata.cmds)
    commands_end = .;
    dumpcommands_start = .;
    *(.rdata.dumpcmds)
    dumpcommands_end = .;
  }

  .data : {
    /* List of haret variables */
    vars_start = .;
    *(.data.vars)
    vars_end = .;
  }
}

/* Create two symbols around all code. */
_text_start = ADDR(.text);
_text_end = ADDR(.text) + SIZEOF(.text);
