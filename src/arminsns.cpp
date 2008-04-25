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
#include "exceptions.h" // TRY_EXCEPTION_HANDLER
#include "arminsns.h"


/****************************************************************
 * Coprocessor accessors
 ****************************************************************/

// Clean D cache line.
DEF_SETCPRATTR(set_cleanDCache, p15, 0, c7, c10, 1, __irq, "memory")
// Drain write buffer.
DEF_SETCPRATTR(set_drainWrites, p15, 0, c7, c10, 4, __irq, "memory")
// Invalidate I cache line.
DEF_SETCPRATTR(set_invICache, p15, 0, c7, c5, 1, __irq, "memory")

uint32 __irq
runArmInsn(uint32 insn, uint32 r0)
{
    // Self-modified code
    uint32 selfmod[3], *sm = selfmod;
    if ((uint32)sm & 0x7)
        sm++;
    sm[0] = insn;
    sm[1] = 0xe1a0f00e; // mov pc,lr
    uint32 mvamod = MVAddr_irq((uint32)sm);
    uint32 ret = 0xffffffff;

    set_cleanDCache(mvamod);
    set_drainWrites(0);
    ret = ((uint32 (*)(uint32))mvamod)(r0);
    set_invICache(mvamod);

    return ret;
}

uint32
buildArmCPInsn(uint setval, uint cp, uint op1, uint CRn, uint CRm, uint op2)
{
    if (cp > 15 || op1 > 7 || CRn > 15 || CRm > 15 || op2 > 7)
        return 0;

    // Create the instruction
    uint32 insn = 0xee100010;  // MRC
    if (setval)
        insn = 0xee000010; // MCR
    insn |= (op1<<21) | (CRn<<16) | (cp << 8) | (op2<<5) | CRm;
    return insn;
}

uint32
buildArmMRSInsn(uint spsr)
{
    // Create the instruction
    uint32 insn = 0xe10f0000;  // MRS
    if (spsr)
        insn |= 1<<22;
    return insn;
}

// Read or modify a coprocessor register
static uint32
getSetCP(uint setval, uint cp, uint op1, uint CRn, uint CRm, uint op2, uint val)
{
    uint32 ret = 0xffffffff;
    uint32 insn = buildArmCPInsn(setval, cp, op1, CRn, CRm, op2);
    if (!insn)
        return ret;

    // Run the instruction
    TRY_EXCEPTION_HANDLER {
        ret = runArmInsn(insn, val);
    } CATCH_EXCEPTION_HANDLER {
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
    if (newContext == 0x10)
        // Get regs from system mode instead of user mode
        newContext = 0x1f;
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
    if (newContext == 0x10)
        // Get regs from system mode instead of user mode
        newContext = 0x1f;
    if (nr == 13)
        newContext |= (1<<30); // Z bit
    uint32 temp;
    asm volatile("mrs %0, cpsr   @ Get current cpsr\n"
                 "msr cpsr, %2   @ Change processor mode\n"
                 "moveq r13, %1  @ Set r13\n"
                 "movne r14, %1  @ Set r14\n"
                 "msr cpsr, %0   @ Restore processor mode"
                 : "=&r" (temp)
                 : "r" (val), "r" (newContext));
}

// Lookup an assembler name for an instruction - this is incomplete.
const char *
getInsnName(uint32 insn)
{
    const char *iname = "?";
    int isLoad = Lbit(insn);
    if ((insn & 0x0C000000) == 0x04000000) {
        if (isLoad) {
            if (Bbit(insn))
                iname = "ldrb";
            else
                iname = "ldr";
        } else {
            if (Bbit(insn))
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
    } else if ((insn & 0x0E000000) == 0x08000000) {
        // multiword instructions
        const char *multiIns[] = {"stmda", "stmdb", "stmia", "stmib",
                                  "ldmda", "ldmdb", "ldmia", "ldmib"};
        int isPre = Pbit(insn);
        int isInc = Ubit(insn);
        iname = multiIns[isLoad*4 + isInc*2 + isPre];
    }

    return iname;
}
