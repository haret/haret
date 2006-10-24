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
    uint32 vaddr = 0; // virtual address
    RawFrameBufferInfo frameBufferInfo;
    
    HDC hdc = GetDC (NULL);
    int result = ExtEscape (hdc, GETRAWFRAMEBUFFER, 0, NULL,
                            sizeof (RawFrameBufferInfo), (char*)&frameBufferInfo);
    ReleaseDC (NULL, hdc);
    
    if (result > 0)
      vaddr = (uint32)frameBufferInfo.pFramePointer;
    else if (videoBeginDraw ())
    {
      vaddr = (uint32)vram;
      videoEndDraw ();
    }

    if (vaddr != 0)
    {
      return memVirtToPhys (vaddr);
    }

    return 0;
} 

bool videoBeginDraw ()
{
    RawFrameBufferInfo frameBufferInfo;
    
    HDC hdc = GetDC (NULL);
    int result = ExtEscape (hdc, GETRAWFRAMEBUFFER, 0, NULL,
                            sizeof (RawFrameBufferInfo), (char*)&frameBufferInfo);
    ReleaseDC (NULL, hdc);
    
    if (result > 0)
    {
      vram = (uint16 *)frameBufferInfo.pFramePointer;
      videoW = frameBufferInfo.cxPixels;
      videoH = frameBufferInfo.cyPixels;
      return TRUE;
    } 
    else
    {
      if (GXOpenDisplay (GetDesktopWindow (), 0) == 0)
        return FALSE;
      vram = (uint16 *)GXBeginDraw ();
      videoW = GetSystemMetrics (SM_CXSCREEN);
      videoH = GetSystemMetrics (SM_CYSCREEN);
      return TRUE;
    }
}

void videoEndDraw ()
{
  if (vram)
    vram = NULL;
}

void videoBitmap::load (uint ResourceID)
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
