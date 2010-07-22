#ifndef MEMCMDS_H
#define MEMCMDS_H

//MMU Level2 table dump routine used by MMU merge
int parseL1Entry(uint32 mb, uint32 l1d, uint32 pL1, 
       uint32 l1only, uint32 showall, uint32 start, uint32 size );
void memPhysFill(uint32 paddr, uint32 wcount, uint32 value, int wordsize);

#endif // MEMCMDS_H
