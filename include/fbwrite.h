#ifndef _FBWRITE_H
#define _FBWRITE_H

// Functions for writing strings directly to the frame buffer

#include "xtypes.h" // uint16, uint32

#define FONTDATAMAX 1536

typedef void (__stdcall *fb_putc_t)(struct fbinfo *, char c);

struct fbinfo {
    uint16 *fb;
    const unsigned char *fonts;
    int scrx, scry, maxx, maxy;
    int x, y;

    // This field contains a pointer to the fb_putc() function. Having this dynamic
    // allows an arch to override this function, for example to support output
    // via a serial port or to perform a DMA operation to update the screen.
    // Keep state in the 64-byte data variable provided, so that it stays within
    // the preloader struct. Any data outside the preloader struct is inaccessible
    // when booting.
    fb_putc_t putcFunc;
    unsigned char putcFuncData[64];
};

void fb_putc(fbinfo *fbi, char c);
void fb_printf(fbinfo *fbi, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));
void fb_clear(fbinfo *fbi);
void fb_init(fbinfo *fbi);

extern const unsigned char fontdata_mini_4x6[FONTDATAMAX];

#endif // _FBWRITE_H

