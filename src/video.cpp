/*
    Video Chip access
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <windows.h>
#include <gx.h> // GXOpenDisplay

#include "xtypes.h"
#include "haret.h" // hInst
#include "memory.h" // memVirtToPhys
#include "output.h" // Complain
#include "script.h" // REG_VAR_ROFUNC
#include "lateload.h" // LATE_LOAD
#include "video.h"

uint16 *vram = NULL;
uint videoW, videoH;

// Is there another way to find this not involving GAPI?
uint16 *vidGetVirtVRAM()
{
    uint16 *vaddr = 0; // virtual address
    RawFrameBufferInfo frameBufferInfo;
    
    HDC hdc = GetDC (NULL);
    int result = ExtEscape (hdc, GETRAWFRAMEBUFFER, 0, NULL,
                            sizeof (RawFrameBufferInfo), (char*)&frameBufferInfo);
    ReleaseDC (NULL, hdc);
    
    if (result > 0)
      vaddr = (uint16*)frameBufferInfo.pFramePointer;
    else if (videoBeginDraw ())
    {
      vaddr = vram;
      videoEndDraw ();
    }

    return vaddr;
}

// This is the way to find the framebuffer information not involving GAPI.
uint32 vidGetVRAM()
{
    return memVirtToPhys((uint32)vidGetVirtVRAM());
}
REG_VAR_ROFUNC(0, "VRAM", vidGetVRAM, 0, "Video Memory physical address")

__LATE_LOAD(GXOpenDisplay, L"?GXOpenDisplay@@YAHPAUHWND__@@K@Z", L"gx")
__LATE_LOAD(GXBeginDraw, L"?GXBeginDraw@@YAPAXXZ", L"gx")

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
      if (! late_GXOpenDisplay || ! late_GXBeginDraw)
        return FALSE;
      if (late_GXOpenDisplay(GetDesktopWindow (), 0) == 0)
        return FALSE;
      vram = (uint16 *)late_GXBeginDraw();
      videoW = GetSystemMetrics(SM_CXSCREEN);
      videoH = GetSystemMetrics(SM_CYSCREEN);
      return TRUE;
    }
}

void videoEndDraw ()
{
  if (vram)
    vram = NULL;
}

// Milliseconds to sleep for nicer animation :-)
static uint32 bootSpeed = 5;
REG_VAR_INT(0, "BOOTSPD", bootSpeed
            , "Boot animation speed, usec/scanline (0-no delay)")

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
