/*
    CPU & coprocessor manipulations
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include "xtypes.h"
#include "machines.h" // Mach
#include "output.h" // Output
#include "cpu.h" // take_control
#include "script.h" // REG_DUMP

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
        if (setval)
            ((void (*)(uint32))&selfmod)(val);
        else
            ret = ((uint32 (*)())&selfmod)();
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
        Output(C_ERROR "line %d: Expected <cp>", ScriptLine);
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
