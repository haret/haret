/* Code to manipulate ARM instructions.
 *
 * (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
 *
 * This file may be distributed under the terms of the GNU GPL license.
 */

#include "machines.h" // Mach
#include "output.h" // Output
#include "irq.h" // __irq
#include "script.h" // REG_VAR_RWFUNC
#include "arminsns.h"


/****************************************************************
 * Coprocessor accessors
 ****************************************************************/

// Self-modified code
static uint32 selfmod[2] = {
    0xee100010,  // mrc pX,0,r0,crX,cr0,0
    0xe1a0f00e,  // mov pc,lr
};

// Read or modify a coprocessor register
static uint32
getSetCP(uint setval, uint cp, uint op1, uint CRn, uint CRm, uint op2, uint val)
{
    if (cp > 15 || op1 > 7 || CRn > 15 || CRm > 15 || op2 > 7)
        return 0xffffffff;

    // Create the instruction
    uint32 insn = 0xee100010;  // MRC
    if (setval)
        insn = 0xee000010; // MCR
    selfmod[0] = (insn | (op1<<21) | (CRn<<16) | (cp << 8) | (op2<<5) | CRm);

    // Flush the CPU caches.
    take_control();
    Mach->flushCache();
    return_control();

    // Run the instruction
    uint32 ret = 0xffffffff;
    try {
        ret = ((uint32 (*)(uint32))&selfmod)(val);
    } catch (...) {
        Output(C_ERROR "EXCEPTION on access to coprocessor %d register %d"
               , cp, CRn);
    }

    return ret;
}

static uint32
cpuScrCP(bool setval, uint32 *args, uint32 val)
{
    return getSetCP(setval, args[0], args[1], args[2], args[3], args[4], val);
}
REG_VAR_RWFUNC(0, "CP", cpuScrCP, 5, "Coprocessor Register access")

static void
cpuDumpCP(const char *tok, const char *args)
{
    uint32 cp;
    if (!get_expression(&args, &cp)) {
        ScriptError("Expected <cp>");
        return;
    }

    if (cp > 15) {
        Output(C_ERROR "Coprocessor number is a number in range 0..15");
        return;
    }

    for (int i = 0; i < 8; i++)
        Output("c%02d: %08x | c%02d: %08x"
               , i, getSetCP(0, cp, 0, i, 0, 0, 0)
               , i + 8, getSetCP(0, cp, 0, i + 8, 0, 0, 0));
}
REG_DUMP(0, "CP", cpuDumpCP,
         "CP <cp>\n"
         "  Show the value of 16 coprocessor registers (arg = coproc number)")


/****************************************************************
 * Instruction/register analysis
 ****************************************************************/

// Get the SPSR register
static inline uint32 __irq get_SPSR(void) {
    uint32 val;
    asm volatile("mrs %0, spsr" : "=r" (val));
    return val;
}

// Return the value of a given register.
uint32 __irq
getReg(struct irqregs *regs, uint32 nr)
{
    if (nr < 13)
        return regs->regs[nr];
    if (nr >= 15)
        return regs->old_pc;

    // In order to access the r13/r14 registers, it is necessary
    // to switch contexts, copy the registers to low order
    // registers, and then switch context back.
    uint32 newContext = get_SPSR();
    newContext &= 0x1f; // Extract mode bits.
    newContext |= (1<<6)|(1<<7); // Disable interrupts
    uint32 temp, hiregs[2];
    asm volatile("mrs %0, cpsr @ Get current cpsr\n"
                 "msr cpsr, %3 @ Change processor mode\n"
                 "mov %1, r13  @ Get r13\n"
                 "mov %2, r14  @ Get r14\n"
                 "msr cpsr, %0 @ Restore processor mode"
                 : "=&r" (temp), "=r" (hiregs[0]), "=r" (hiregs[1])
                 : "r" (newContext));
    return hiregs[nr-13];
}

// Set the value of a register.
void __irq
setReg(struct irqregs *regs, uint32 nr, uint32 val)
{
    if (nr < 13) {
        regs->regs[nr] = val;
        return;
    }
    if (nr >= 15) {
        regs->old_pc = val;
        return;
    }

    // In order to access the r13/r14 registers, it is necessary to
    // switch contexts, set the register, and then switch context
    // back.
    uint32 newContext = get_SPSR();
    newContext &= 0x1f; // Extract mode bits.
    newContext |= (1<<6)|(1<<7); // Disable interrupts
    if (nr == 13)
        newContext |= (1<<30); // Z bit
    uint32 temp;
    asm volatile("mrs %0, cpsr   @ Get current cpsr\n"
                 "msr cpsr, %2   @ Change processor mode\n"
                 "moveq r13, %1  @ Set r13\n"
                 "movne r14, %1  @ Set r14\n"
                 "msr cpsr, %0   @ Restore processor mode"
                 : "=&r" (temp), "=r" (val)
                 : "r" (newContext));
}

// Lookup an assembler name for an instruction - this is incomplete.
const char *
getInsnName(uint32 insn)
{
    const char *iname = "?";
    int isLoad = insn & 0x00100000;
    if ((insn & 0x0C000000) == 0x04000000) {
        if (isLoad) {
            if (insn & (1<<22))
                iname = "ldrb";
            else
                iname = "ldr";
        } else {
            if (insn & (1<<22))
                iname = "strb";
            else
                iname = "str";
        }
    } else if ((insn & 0x0E000000) == 0x00000000) {
        int lowbyte = insn & 0xF0;
        if (isLoad) {
            if (lowbyte == 0xb0)
                iname = "ldrh";
            else if (lowbyte == 0xd0)
                iname = "ldrsb";
            else if (lowbyte == 0xf0)
                iname = "ldrsh";
        } else {
            if (lowbyte == 0xb0)
                iname = "strh";
            else if (lowbyte == 0x90)
                iname = "swp?";
        }
    }

    return iname;
}
