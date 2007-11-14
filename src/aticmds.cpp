/* Commands to fiddle with the ATI FB via the ati dll.
 *
 *
 * For conditions of use see file COPYING
 */

#include "lateload.h" // LATE_LOAD
#include "output.h" // Output, Screen
#include "script.h" // REG_CMD
#include "memory.h" // memVirtToPhys

struct Asic;

struct DevInfo {
        char Name[0x50];
        char Version[0x50];
        int Revision;
        int ChipId;
        int RevisionId;
        int TotalMemory;
        int BusInterfaceMode;
        int InternalMemSize;
        int ExternalMemSize;
        int Caps1;
        int Caps2;
        int Caps3;
        int Caps4;
        char Pad[128];
};

typedef struct
{
	int Width;
	int Height;
	int PixelFormat;
	int Frequency;
	int Rotation;
	int Mirror;
} modeinfo;

extern "C" {
    int AhiInit(int);
    int AhiDevEnum(struct Asic**, DevInfo*, int asicnumber);
int AhiDevOpen(struct Ctx**, Asic*, const char* name, int);
int AhiDevClose(struct Ctx*);
int AhiDispSurfGet(struct Ctx*, struct Surf**);
int AhiSurfAlloc(struct Ctx*, struct Surf**, int size[2], int type, int mode);
int AhiSurfFree(struct Ctx*, struct Surf*);
int AhiSurfLock(struct Ctx*, struct Surf*, unsigned short **addr, int plane);
int AhiSurfUnlock(struct Ctx*, struct Surf*);
int AhiDrawSurfSrcSet(struct Ctx*, struct Surf*, int plane);
int AhiDrawSurfDstSet(struct Ctx*, struct Surf*, int plane);
int AhiDrawRopSet(struct Ctx*, int rop);
int AhiDrawBitBlt(struct Ctx*, const struct Rect*, const struct Pos*);
int AhiDrawIdle(struct Ctx*, int);
//int AhiSurfInfo(struct Ctx*, struct Surf*, SurfInfo*);
int AhiDispModeGet(struct Ctx*, modeinfo*);
int AhiPwrModeSet(struct Ctx*, int, int, int);
int AhiDevRegRead(struct Ctx*,uint32 *regval,int ,int reg );
int AhiDevRegWrite(struct Ctx*,uint32 *regval,int ,int reg );
}

LATE_LOAD(AhiInit, "ace_ddi")
LATE_LOAD(AhiDevEnum, "ace_ddi")
LATE_LOAD(AhiDevOpen, "ace_ddi")
LATE_LOAD(AhiDevClose, "ace_ddi")
LATE_LOAD(AhiDispModeGet, "ace_ddi")
LATE_LOAD(AhiPwrModeSet, "ace_ddi")
LATE_LOAD(AhiDevRegRead, "ace_ddi")
LATE_LOAD(AhiDevRegWrite, "ace_ddi")

static int AtiAvail() {
    return late_AhiInit && late_AhiDevEnum && late_AhiDevOpen && late_AhiDevClose && late_AhiDispModeGet && late_AhiPwrModeSet && late_AhiDevRegRead && late_AhiDevRegWrite;
}

static void
atidbg(const char *cmd, const char *args)
{
    DevInfo devinfo;
    struct Asic *asic;
    struct Ctx  *ctx;

    modeinfo	modeinfo;
    uint32	regval;

    int ret = late_AhiInit(0x0); /* 0x11, !0x100 */

    ret = late_AhiDevEnum(&asic, &devinfo, 0);
    Output ("ATI asic* =0x%p / 0x%8.8x ret=%d\n", asic, memVirtToPhys((uint32)asic),ret);

    Screen("ATI ChipId           = 0x%8.8x", devinfo.ChipId);
    Output("ATI Revision         = 0x%8.8x", devinfo.RevisionId);
    Output("ATI RevisionId       = 0x%8.8x", devinfo.RevisionId);
    Output("ATI BusInterfaceMode = 0x%8.8x", devinfo.BusInterfaceMode);
    Screen("ATI InternalMemSize  = 0x%8.8x", devinfo.InternalMemSize);
    Screen("ATI ExternalMemSize  = 0x%8.8x", devinfo.ExternalMemSize);
    Output("ATI Caps1            = 0x%8.8x", devinfo.Caps1);
    Output("ATI Caps2            = 0x%8.8x", devinfo.Caps2);
    Output("ATI Caps3            = 0x%8.8x", devinfo.Caps3);
    Output("ATI Caps4            = 0x%8.8x", devinfo.Caps4);

    ret = late_AhiDevOpen(&ctx, asic, "haret", 2);
    Output ("ATI ctx* =0x%p / 0x%8.8x ret=%d\n", ctx, memVirtToPhys((uint32)ctx),ret);

    late_AhiDispModeGet(ctx,&modeinfo);
    Output ("ATI Width            = %d", modeinfo.Width);
    Screen ("ATI Width            = %d", modeinfo.Width);
    Output ("ATI Height           = %d", modeinfo.Height);
    Screen ("ATI Height           = %d", modeinfo.Height);
    Output ("ATI PixelFormat      = 0x%x", modeinfo.PixelFormat);
    Output ("ATI Frequency        = %d", modeinfo.Frequency);
    Screen ("ATI Frequency        = %d", modeinfo.Frequency);
    Output ("ATI Rotation         = %d", modeinfo.Rotation);
    Output ("ATI Mirror           = %d", modeinfo.Mirror);

    regval=0;
    late_AhiDevRegRead(ctx,&regval,1,0x84);
    Output ("ATI mmPLL_REF_FB_DIV       = 0x%8.8x", regval);

 uint32 ahdisp, avdisp, ghdisp, gvdisp, total, pll, sclk, pclk;
 int	aup,adown,aleft,aright,gup,gdown,gleft,gright;
 int	xrest,yrest;

    late_AhiDevRegRead(ctx,&total,1,0x420);
    late_AhiDevRegRead(ctx,&ahdisp,1,0x424);
    late_AhiDevRegRead(ctx,&avdisp,1,0x428);

    late_AhiDevRegRead(ctx,&ghdisp,1,0x42c);
    late_AhiDevRegRead(ctx,&gvdisp,1,0x430);
 
 xrest=total&0xffff;
 yrest=(total>>16)&0xffff;
 
 aleft =ahdisp&0x3ff;
 aright=(ahdisp>>16)&0x3ff;
 aup   =avdisp&0x3ff;
 adown =(avdisp>>16)&0x3ff;

 gleft =ghdisp&0x3ff;
 gright=(ghdisp>>16)&0x3ff;
 gup   =gvdisp&0x3ff;
 gdown =(gvdisp>>16)&0x3ff;

 Output("margins: xrest=%d yrest=%d left=%d/%d right=%d/%d up=%d/%d down=%d/%d\n",
  xrest,yrest,aleft,gleft,xrest-aright,xrest-gright,aup,gup,yrest-adown,yrest-gdown);

    late_AhiDevRegRead(ctx,&pll,1,0x84);

 int M, N_int, N_frac,lock_time;

 M=pll&0xf;
 N_int=(pll>>8)&0x3f;
 N_frac=(pll>>16)&0x7;
 lock_time=(pll>>24)&0xf;

 Output("pll: ref_div(M)=%d div_int(N_int)=%d div_frac(N_frac)=%d lock_time=%d Fout=%d*xtal\n",
  M,N_int,N_frac,lock_time,((N_int+1)*8 + N_frac)/(M+1)/8 );

    late_AhiDevRegRead(ctx,&sclk,1,0x8c);

 int src_sel, sclk_div_fast, sclk_div_slow;

 src_sel=sclk&0x3;
 sclk_div_fast=(sclk>>4)&0xf;
 sclk_div_slow=(sclk>>11)&0xf;

 Output("sclk: sysclk_src=%d sysclk_div: fast=%d slow=%d\n",
  src_sel, sclk_div_fast,sclk_div_slow);

    late_AhiDevRegRead(ctx,&pclk,1,0x90);

 int pclk_div;

 src_sel=pclk&0x3;
 pclk_div=(pclk>>4)&0xf;

 Output("pclk: pixclk_src=%d pixclk_div=%d\n",
  src_sel, pclk_div);

}
REG_CMD(AtiAvail, "ATIDBG", atidbg,
        "ATIDBG\n"
        "  Return info on ATI FB using ati dll.")
