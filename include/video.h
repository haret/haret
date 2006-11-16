/*
    Video Controller manipulation functions
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _VIDEO_H
#define _VIDEO_H

#include <windows.h>

// This class can be used to load a bitmap from a resource and display it
class videoBitmap
{
  HRSRC rh;
  uint8 *data;
  BITMAPINFO *bmi;
  uint8 *pixels;
public:
  // Load the bitmap
  void load (uint ResourceID);
  // No destructor
  //~videoBitmap ();
  // Get bitmap width
  uint GetWidth () { return bmi->bmiHeader.biWidth; }
  // Get bitmap height
  uint GetHeight () { return bmi->bmiHeader.biHeight; }
  // Draw a single line from bitmap to given position
  // (lines are counted bottom-up)
  void DrawLine (uint x, uint y, uint lineno);
  // Draw the entire bitmap
  void Draw (uint x, uint y);
};

#ifndef GETRAWFRAMEBUFFER
  #define GETRAWFRAMEBUFFER 0x00020001
  typedef struct _RawFrameBufferInfo
  {
    WORD wFormat;
    WORD wBPP;
    VOID *pFramePointer;
    int	cxStride;
    int	cyStride;
        int cxPixels;
        int cyPixels;
  } RawFrameBufferInfo;
#endif

// Return the virtual address of video RAM
extern uint16 *vidGetVirtVRAM();
// Return the physical address of video RAM
extern uint32 vidGetVRAM ();

// The pointer to video memory (valid between BeginDraw/EndDraw)
extern uint16 *vram;
// Screen width and height (assigned by BeginDraw)
extern uint videoW, videoH;
// Begin drawing directly to screen
bool videoBeginDraw ();
// Finish drawing directly to screen
void videoEndDraw ();

#endif /* _VIDEO_H */
