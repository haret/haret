/* Write strings directly to the framebuffer.
 *
 * (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
 *
 * This file may be distributed under the terms of the GNU GPL license.
 */

#include "output.h" // Output
#include "linboot.h" // __preload
#include "video.h" // videoW, videoH

#include "fbwrite.h"

#define FONTWIDTH 4
#define FONTHEIGHT 6
#define BPP 2

// Write a character to the screen.
static void __preload
blit_char(fbinfo *fbi, char c)
{
    if (fbi->x > fbi->maxx || fbi->y > fbi->maxy)
        return;

    const unsigned char *font = &fbi->fonts[c * FONTHEIGHT];
    uint16 *fbpos = &fbi->fb[fbi->scrx * fbi->y * FONTHEIGHT
                             + fbi->x * FONTWIDTH];
    for (int y=0; y<FONTHEIGHT; y++) {
        for (int x=0; x<FONTWIDTH; x++) {
            if (font[y] & (1<<(FONTWIDTH-x-1)))
                fbpos[x] = 0xFFFF;
        }
        fbpos += fbi->scrx;
    }
}

static void __preload
goNewLine(fbinfo *fbi)
{
    fbi->x = 0;
    if (fbi->y < fbi->maxy-1) {
        fbi->y++;
        return;
    }
    int linebytes = fbi->scrx * FONTHEIGHT;
    do_copy((char *)fbi->fb
            , (char *)&fbi->fb[linebytes]
            , (fbi->scrx * fbi->scry - linebytes) * BPP);
}

// Write a string to the framebuffer.
void __preload
fb_puts(fbinfo *fbi, const char *s)
{
    if (!fbi->fb)
        return;

    for (; *s; s++) {
        if (*s == '\n') {
            goNewLine(fbi);
            continue;
        }
        blit_char(fbi, *s);
        fbi->x++;
        if (fbi->x >= fbi->maxx)
            goNewLine(fbi);
    }
}

// Clear the screen.
void
fb_clear(fbinfo *fbi)
{
    if (!fbi->fb)
        return;
    memset(fbi->fb, 0, fbi->scrx * fbi->scry * BPP);
    fbi->x = fbi->y = 0;
}

// Initialize an fbi structure.
void
fb_init(fbinfo *fbi)
{
    memset(fbi, 0, sizeof(*fbi));
    fbi->fb = vidGetVirtVRAM();
    fbi->fonts = fontdata_mini_4x6;

    fbi->scrx = videoW;
    fbi->scry = videoH;
    fbi->maxx = videoW / FONTWIDTH;
    fbi->maxy = videoH / FONTHEIGHT;

    Output("Video buffer at %p sx=%d sy=%d mx=%d my=%d"
           , fbi->fb, fbi->scrx, fbi->scry, fbi->maxx, fbi->maxy);
}
