/* Find and hook WinCE code vectors.
 *
 * (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
 *
 * This file may be distributed under the terms of the GNU GPL license.
 */

#include <stdlib.h> // malloc
#include <string.h> // memcpy

#include "script.h" // REG_VAR_INT
#include "memory.h" // memVirtToPhys
#include "output.h" // Output
#include "cpu.h" // take_control
#include "machines.h" // Mach
#include "winvectors.h"

static const uint32 VADDR_IRQTABLE=0xffff0000;

// Locate a WinCE exception handler.  This assumes the handler is
// setup in a manor that wince has been observed to do in the past.
uint32 *
findWinCEirq(uint32 offset)
{
    // Map in the IRQ vector tables in a place that we can be sure
    // there is full read/write access to it.
    uint8 *irq_table = memPhysMap(memVirtToPhys(VADDR_IRQTABLE));

    uint32 irq_ins = *(uint32*)&irq_table[offset];
    if ((irq_ins & 0xfffff000) != 0xe59ff000) {
        // We only know how to handle LDR PC, #XXX instructions.
        Output(C_INFO "Unknown irq instruction %08x", irq_ins);
        return NULL;
    }
    uint32 ins_offset = (irq_ins & 0xFFF) + 8;
    //Output("Ins=%08x ins_offset=%08x", irq_ins, ins_offset);
    return (uint32 *)(&irq_table[offset + ins_offset]);
}

static uint32 winceResumeAddr = 0xffffffff;
REG_VAR_INT(0, "RESUMEADDR", winceResumeAddr
            , "Location of wince resume handler")
static uint32 *ResumePtr, OldResume[2];
static struct stackJumper_s *ResumeSJ;

// Setup wince to resume into a haret handler.
int
hookResume(uint32 handler, uint32 stack, uint32 data, int complain)
{
    if (winceResumeAddr == 0xffffffff) {
        if (! complain)
            return 0;
        Output(C_ERROR "Please specify WinCE physical resume address");
        return -1;
    }

    // Lookup wince resume address and verify it looks sane.
    ResumePtr = (uint32*)memPhysMap(winceResumeAddr);
    if (!ResumePtr) {
        Output(C_ERROR "Could not map resume addr %08x", winceResumeAddr);
        return -1;
    }
    // Check for "b 0x41000 ; 0x0" at the address.
    OldResume[0] = ResumePtr[0];
    OldResume[1] = ResumePtr[1];
    if (OldResume[0] != 0xea0003fe || OldResume[1] != 0x0) {
        Output(C_ERROR "Unexpected resume vector. (%08x %08x)"
               , OldResume[0], OldResume[1]);
        goto fail;
    }

    // Allocate and setup C code trampoline
    ResumeSJ = (stackJumper_s*)malloc(sizeof(*ResumeSJ)*2);
    if (! ResumeSJ) {
        Output("Unable to allocate memory for trampoline");
        goto fail;
    }
    if (PAGE_ALIGN((uint32)ResumeSJ) != PAGE_ALIGN((uint32)&ResumeSJ[1]))
        // Spans a page - move it to start of page.
        ResumeSJ = (stackJumper_s*)PAGE_ALIGN((uint32)ResumeSJ);
    memcpy(ResumeSJ, &stackJumper, sizeof(*ResumeSJ));
    ResumeSJ->stack = stack;
    ResumeSJ->data = data;
    ResumeSJ->execCode = handler;
    ResumeSJ->returnCode = winceResumeAddr + 0x1000;
    handler = retryVirtToPhys((uint32)ResumeSJ->asm_handler);

    Output("Redirecting resume (%p) to %08x", ResumePtr, handler);

    // Overwrite the resume vector.
    take_control();
    Mach->flushCache();
    ResumePtr[0] = 0xe51ff004; // ldr pc, [pc, #-4]
    ResumePtr[1] = handler;
    return_control();
    return 0;
fail:
    free(ResumeSJ);
    ResumeSJ = NULL;
    ResumePtr = NULL;
    return -1;
}

void
unhookResume()
{
    if (!ResumePtr)
        // Nothing to do.
        return;
    Output("Restoring resume (%p) to original (%08x %08x)"
           , ResumePtr, OldResume[0], OldResume[1]);
    take_control();
    Mach->flushCache();
    ResumePtr[0] = OldResume[0];
    ResumePtr[1] = OldResume[1];
    return_control();

    free(ResumeSJ);
    ResumeSJ = NULL;
    ResumePtr = NULL;
}
