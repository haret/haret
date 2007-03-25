// Functions for writing strings directly to the frame buffer

#include "xtypes.h" // uint16, uint32

#define FONTDATAMAX 1536

struct fbinfo {
    uint16 *fb;
    const unsigned char *fonts;
    int scrx, scry, maxx, maxy;
    int x, y;
};

void fb_puts(fbinfo *fbi, const char *s);
void fb_clear(fbinfo *fbi);
void fb_init(fbinfo *fbi);

extern const unsigned char fontdata_mini_4x6[FONTDATAMAX];
