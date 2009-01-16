#include <string.h> // strncmp
#include "arch-arm.h" // cpuFlushCache_arm926
#include "script.h" // runMemScript
#include "arch-imx.h"


/****************************************************************
 * i.MX21
 ****************************************************************/

MachineIMX21::MachineIMX21()
{
	name = "FreeScale i.MX21";
	archname = "iMX21";
	CPUInfo[0] = L"i.MX21";
	flushCache = cpuFlushCache_arm926;
}

void
MachineIMX21::init()
{
	runMemScript(
		"set ramaddr 0xc0000000\n"
		// IRQs
		"addlist IRQS p2v(0x10040048) 0 32 0\n"
		"addlist IRQS p2v(0x1004004c) 0 32 0\n"
	);
}


// Returns true if the current machine was found to be iMX based.
int
testIMX()
{
	return strncmp(Mach->archname, "iMX", 3) == 0;
}

REGMACHINE(MachineIMX21)
