/*
 * Video Chip access
 * (C) Copyright 2008 Kevin O'Connor <kevin@koconnor.net>
 * Copyright (C) 2003 Andrew Zabolotny
 *
 * For conditions of use see file COPYING
 */

#include <windows.h> // ExtEscape
#include <gx.h> // GXOpenDisplay

#include "memory.h" // retryVirtToPhys
#include "output.h" // Output
#include "script.h" // REG_VAR_ROFUNC
#include "lateload.h" // LATE_LOAD
#include "video.h"


/****************************************************************
 * ExtEscape info
 ****************************************************************/

#define GETRAWFRAMEBUFFER 0x00020001
typedef struct _RawFrameBufferInfo {
    WORD wFormat;
    WORD wBPP;
    VOID *pFramePointer;
    int cxStride;
    int cyStride;
    int cxPixels;
    int cyPixels;
} RawFrameBufferInfo;

#define GETGXINFO 0x00020000
typedef struct {
    long Version; //00 (should filled with 100 before calling ExtEscape)
    void *pvFrameBuffer; //04
    unsigned long cbStride; //08
    unsigned long cxWidth; //0c
    unsigned long cyHeight; //10
    unsigned long cBPP; //14
    unsigned long ffFormat; //18
    char Unused[0x84-7*4];
} GXDeviceInfo;


/****************************************************************
 * GX dll binding
 ****************************************************************/

static uint32 returnZero(void) { return 0; }

__LATE_LOAD(GXOpenDisplay, L"?GXOpenDisplay@@YAHPAUHWND__@@K@Z", L"gx"
            , &returnZero)
__LATE_LOAD(GXCloseDisplay, L"?GXCloseDisplay@@YAHXZ", L"gx", &returnZero)
__LATE_LOAD(GXBeginDraw, L"?GXBeginDraw@@YAPAXXZ", L"gx", &returnZero)
__LATE_LOAD(GXEndDraw, L"?GXEndDraw@@YAHXZ", L"gx", &returnZero)


/****************************************************************
 * Framebuffer detection code
 ****************************************************************/

// Screen width and height
uint videoW, videoH;

// Return the virtual address of video RAM
uint16 *vidGetVirtVRAM()
{
    // Try GETRAWFRAMEBUFFER method
    RawFrameBufferInfo frameBufferInfo;
    HDC hdc = GetDC(NULL);
    int ret = ExtEscape(hdc, GETRAWFRAMEBUFFER, 0, NULL,
                        sizeof(frameBufferInfo), (char*)&frameBufferInfo);
    ReleaseDC(NULL, hdc);
    if (ret > 0) {
        videoW = frameBufferInfo.cxPixels;
        videoH = frameBufferInfo.cyPixels;
        return (uint16*)frameBufferInfo.pFramePointer;
    }

    // Try GAPI method.
    if (late_GXOpenDisplay(GetDesktopWindow(), 0)) {
        uint16 *vaddr = (uint16 *)late_GXBeginDraw();
        videoW = GetSystemMetrics(SM_CXSCREEN);
        videoH = GetSystemMetrics(SM_CYSCREEN);
        late_GXEndDraw();
        late_GXCloseDisplay();
        return vaddr;
    }

    // Try GETGXINFO method.
    GXDeviceInfo rfb;
    rfb.Version = 100;
    hdc = GetDC(NULL);
    ret = ExtEscape(hdc, GETGXINFO, 0, NULL, sizeof(rfb), (char*)&rfb);
    ReleaseDC(NULL, hdc);
    if (ret > 0) {
        videoW = rfb.cxWidth;
        videoH = rfb.cyHeight;
        return (uint16 *)rfb.pvFrameBuffer;
    }

    Output("Unable to detect frame buffer address");
    return 0;
}

// Return the physical address of video RAM
uint32 vidGetVRAM()
{
    uint16 *vaddr = vidGetVirtVRAM();
    if (!vaddr)
        return 0;
    uint32 res = retryVirtToPhys((uint32)vaddr);
    if (res == (uint32)-1)
        return 0;
    return res;
}

// Script command wrappers
static uint32 cmd_vidGetVRAM(bool, uint32*, uint32) {
    return vidGetVRAM();
}
REG_VAR_ROFUNC(0, "VRAM", cmd_vidGetVRAM, 0, "Video Memory physical address")
static uint32 cmd_vidGetVirtVRAM(bool, uint32*, uint32) {
    return (uint32)vidGetVirtVRAM();
}
REG_VAR_ROFUNC(0, "VIRTVRAM", cmd_vidGetVirtVRAM, 0,
               "Video Memory virtual address")
