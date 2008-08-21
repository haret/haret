/* MMU Table merging
 *
 * (C) Copyright 2008 Oliver Ford <ipaqlinux@oliford.co.uk>
 *
 * This file may be distributed under the terms of the GNU GPL license.
 *
 * Idea is to make a copy of the l1 table and at the start every IRQ we compare it to the current one.
 * Any changes are just written to copy of the table except for writing type 0 (unmapped).
 * We should end up with a table which is the merged copy of anything windows tries to hide.
 * The counts represent the number of times something actually different gets mapped there, so
 * if winCE is just turning it on and off, the count will be 1.
 *
 */
#include <windows.h> // for pkfuncs.h

#include "output.h" // Output
#include "memory.h" // memVirtToPhys
#include "memcmds.h" // parseL1Entry
#include "irq.h"
#include "memcmds.h"

static uint32 MMUMergeStart;
REG_VAR_INT(0,"MMUMergeStart",MMUMergeStart,
            "First entry of MMU level 1 table to watch.")
static uint32 MMUMergeCount;
REG_VAR_INT(0,"MMUMergeCount",MMUMergeCount,
            "Number of MMU level 1 table entries to watch. Making this too big (~> 512) may seriously slow/crash your system!")

static uint32 MMUMergeTargetStart = 0x40000000; //pxa regs
REG_VAR_INT(0,"MMUMergeTargetStart",MMUMergeTargetStart,
            "Physical address to scan the merged table for on completion of wirq.")
static uint32 MMUMergeTargetSize = 0x10000000; //the whole bank
REG_VAR_INT(0,"MMUMergeTargetSize",MMUMergeTargetSize,
            "Size of physical address range to scan the merged table for on completion of wirq")

// setup
int prepMMUMerge(struct irqData *data)
{
	data->mergeTableStart = MMUMergeStart;
	data->mergeTableCount = MMUMergeCount;

	if( !data->mergeTableCount ){
		Output("MMU table merging disabled");
		return 0;
	}
	if( data->mergeTableCount > 4096){
		Output("MMU table merging request too long (max = 4096)");
		data->mergeTableCount = 0;
		return 0;
	}
	
	//make sure we have the mmu table address
	if( !data->mmuVAddr ){
		data->mmuVAddr = (uint32*)memPhysMap(cpuGetMMU());
		if (! data->mmuVAddr) {
			Output("Unable to map MMU table");
			data->mergeTableCount = 0; 
			return -1;
		}
	}
	Output("MMU L1 table merging enabled, watching %i entries starting at entry %i",
		data->mergeTableCount,data->mergeTableStart);
	
	return 0;
}

// start
void startMMUMerge(struct irqData *data)
{ 
	if( !data->mergeTableCount ) return;
	
	//make the initial copy
	memcpy(data->l1Copy,data->mmuVAddr + data->mergeTableStart,0x4000); //make initial copy now (after harets modifications)
	memset(data->l1Changed,0,sizeof(data->l1Changed)); //clear all change flags
}

// stop
void __irq stopMMUMerge(struct irqData *data)
{
	if( !data->mergeTableCount ) return;	
	//anything to do?
}

void dumpMMUMerge(struct irqData *data){
	if( !data->mergeTableCount ) return;

	Output("Dumping MMU Merge table (last change mappings):");
        Output("  Virtual | Physical |   Description |  Flags");
        Output("  address | address  |               |");
        Output("----------+----------+---------------+----------------------");
	//now search the entire table, l1s and l2s, for anything in the range we're interested in
	for(uint i=0;i < data->mergeTableCount;i++){
		if(!data->l1Changed[i])continue;

		uint32 l1d = data->l1Copy[i];
		uint32 mb = data->mergeTableStart + i;
		int count = parseL1Entry(mb,l1d,	//reuse part of the main 'MMU DUMP' code
			0xffffffff,			//pL1=0xffffffff (no sense of previous here)
			0,				//l1only = 0,
			(MMUMergeTargetSize == 0),	//showall = 1 if no range given;
			MMUMergeTargetStart, MMUMergeTargetSize);

		if(count > 0)
			Output("----- End L1 entry %5i (%08x) change count: %i  -----",mb,(mb << 20),data->l1Changed[i]);
	}
}

// perform check and merge
void __irq checkMMUMerge(struct irqData *data )
{
	if( !data->mergeTableCount ) return; 	
	
	uint32 *tableEntry;
	for(uint i=0; i< data->mergeTableCount; i++){ //for each mmu entry
		tableEntry = data->mmuVAddr + (data->mergeTableStart + i);
		if(data->l1Copy[i] != *tableEntry){
			//only interested if its changing to another map, not to unmapped
			if(*tableEntry != 0){
				data->l1Changed[i]++; //count actual changes
				data->l1Copy[i] = *tableEntry;
			}
		}
	}
}
