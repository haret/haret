/* Write strings directly to the framebuffer.
 *
 * (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
 *
 * This file may be distributed under the terms of the GNU GPL license.
 */

#include <stdarg.h> // va_list
#include <string.h> // memset

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

// Write a charcter to the framebuffer.
// Use fbi->putcFunc(...) when calling this function to allow
// arch-specific overrides.
void __preload
fb_putc(fbinfo *fbi, char c)
{
    if (c == '\n') {
        goNewLine(fbi);
        return;
    }
    blit_char(fbi, c);
    fbi->x++;
    if (fbi->x >= fbi->maxx)
        goNewLine(fbi);
}

// Write a string to the framebuffer.
static void __preload
fb_puts(fbinfo *fbi, const char *s)
{
    for (; *s; s++)
        fbi->putcFunc(fbi, *s);
}

// Write an unsigned integer to the screen.
static void __preload
fb_putuint(fbinfo *fbi, uint32 val)
{
    char buf[12];
    char *d = &buf[sizeof(buf) - 1];
    *d-- = '\0';
    for (;;) {
        *d = (val % 10) + '0';
        val /= 10;
        if (!val)
            break;
        d--;
    }
    fb_puts(fbi, d);
}

// Write a single digit hex character to the screen.
static inline void __preload
fb_putsinglehex(fbinfo *fbi, uint32 val)
{
    if (val <= 9)
        val = '0' + val;
    else
        val = 'a' + val - 10;
    fbi->putcFunc(fbi, val);
}

// Write an integer in hexadecimal to the screen.
static void __preload
fb_puthex(fbinfo *fbi, uint32 val)
{
    fb_putsinglehex(fbi, (val >> 28) & 0xf);
    fb_putsinglehex(fbi, (val >> 24) & 0xf);
    fb_putsinglehex(fbi, (val >> 20) & 0xf);
    fb_putsinglehex(fbi, (val >> 16) & 0xf);
    fb_putsinglehex(fbi, (val >> 12) & 0xf);
    fb_putsinglehex(fbi, (val >> 8) & 0xf);
    fb_putsinglehex(fbi, (val >> 4) & 0xf);
    fb_putsinglehex(fbi, (val >> 0) & 0xf);
}

// Write a string to the framebuffer.
void __preload
fb_printf(fbinfo *fbi, const char *fmt, ...)
{
    if (!fbi->fb)
        return;

    va_list args;
    va_start(args, fmt);
    const char *s = fmt;
    for (; *s; s++) {
        if (*s != '%') {
            fbi->putcFunc(fbi, *s);
            continue;
        }
        const char *n = s+1;
        int32 val;
        const char *sarg;
        switch (*n) {
        case '%':
            fbi->putcFunc(fbi, '%');
            break;
        case 'd':
            val = va_arg(args, int32);
            if (val < 0) {
                fbi->putcFunc(fbi, '-');
                val = -val;
            }
            fb_putuint(fbi, val);
            break;
        case 'u':
            val = va_arg(args, int32);
            fb_putuint(fbi, val);
            break;
        case 'x':
            val = va_arg(args, int32);
            fb_puthex(fbi, val);
            break;
        case 's':
            sarg = va_arg(args, const char *);
            fb_puts(fbi, sarg);
            break;
        default:
            fbi->putcFunc(fbi, *s);
            n = s;
        }
        s = n;
    }
    va_end(args);
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

    fbi->putcFunc = &fb_putc;

    Output("Video buffer at %p sx=%d sy=%d mx=%d my=%d"
           , fbi->fb, fbi->scrx, fbi->scry, fbi->maxx, fbi->maxy);
}
