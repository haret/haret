/*
    Video Chip access
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <windows.h>
#include <gx.h>

#include "xtypes.h"
#include "video.h"
#include "haret.h"
#include "memory.h"
#include "output.h"

uint16 *vram = NULL;
uint videoW, videoH;

// Is there another way to find this not involving GAPI?
uint32 vidGetVRAM ()
{
  if (!videoBeginDraw ())
    return 0;

  uint32 vram_addr = (uint32)vram;
  videoEndDraw ();
  return memVirtToPhys (vram_addr);
}

bool videoBeginDraw ()
{
  if (GXOpenDisplay (GetDesktopWindow (), 0) == 0)
    return false;
  vram = (uint16 *)GXBeginDraw ();
  videoW = GetSystemMetrics (SM_CXSCREEN);
  videoH = GetSystemMetrics (SM_CYSCREEN);
  return true;
}

void videoEndDraw ()
{
  if (vram)
  {
    GXEndDraw ();
    GXCloseDisplay ();
    vram = NULL;
  }
}

videoBitmap::videoBitmap (uint ResourceID)
{
  rh = (HRSRC)LoadResource (hInst, FindResource (hInst,
    MAKEINTRESOURCE (ResourceID), RT_BITMAP));
  // 8 bit-per-pixel paletted image format is implicitly assumed
  if (rh)
  {
    data = (uint8 *)LockResource (rh);
    bmi = (BITMAPINFO *)data;
    pixels = (uint8 *)&bmi->bmiColors [bmi->bmiHeader.biClrUsed];
  }
  else
    data = NULL;

  if (!data)
    Complain (C_ERROR ("Failed to load bitmap resource #%d"), ResourceID);
}

// There is no way to unlock/unload a resource ???
//videoBitmap::~videoBitmap ()
//{
//}

void videoBitmap::DrawLine (uint x, uint y, uint lineno)
{
  if (!vram)
    return;

  uint bw = bmi->bmiHeader.biWidth;
  uint abw = ((bw + 3) & ~3);
  uint8 *src = pixels + lineno * abw;
  uint16 *dst = vram + y * videoW + x;

  while (bw--)
  {
    RGBQUAD *pix = bmi->bmiColors + *src++;
    *dst++ = ((uint (pix->rgbRed) >> 3) << 11)
           | ((uint (pix->rgbGreen) >> 2) << 5)
           | ((uint (pix->rgbBlue) >> 3));
  }
}

void videoBitmap::Draw (uint x, uint y)
{
  uint maxy = bmi->bmiHeader.biHeight - 1;
  for (uint dy = 0; dy <= maxy; dy++)
    DrawLine (x, y + dy, maxy - dy);
}
