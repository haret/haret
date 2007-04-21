#ifndef __CBITMAP_H
#define __CBITMAP_H

//#include <values.h> // LONGBITS
#define LONGBITS 32

// Functions for getting and setting bits in a bitmap defined to be of
// type 'long'

// Return the number of 'unsigned longs' necessary to store <num>
// bits.
#define BITMAPSIZE(num) ( ((num) + LONGBITS - 1) / LONGBITS )

// Helper function - return the 'unsigned long' that stores bit <pos>
#define BITMAPPOS(bitset, pos) ((bitset)[(pos)/LONGBITS])
// Helper function - return the bit offset within an unsigned long for
// <pos>
#define BITMAPMASK(pos) ( 1 << ((pos) & (LONGBITS-1)) )

// Set bit <pos> in bitmap <bitset>
#define SETBIT(bitset,pos) do {BITMAPPOS(bitset,pos) |= BITMAPMASK(pos);} while (0)

// Clear bit <pos> in bitmap <bitset>
#define CLEARBIT(bitset,pos) do {BITMAPPOS(bitset,pos) &= ~BITMAPMASK(pos);} while (0)

// Set/Clear the bit at <pos> in bitmap <bitset>
#define ASSIGNBIT(bitset,pos,val) do {BITMAPPOS(bitset,pos) = (BITMAPPOS(bitset,pos) & ~BITMAPMASK(pos)) | ((!!(val)) << ((pos) & (LONGBITS-1)));} while (0)

// Test the bit <pos> in bitmap <bitset>
#define TESTBIT(bitset,pos) (!!(BITMAPPOS(bitset,pos) & BITMAPMASK(pos)))

#endif // cbitmap.h
