/* Commands to fiddle with the ATI FB via the ati dll.
 *
 *
 * For conditions of use see file COPYING
 */

#include "lateload.h" // LATE_LOAD
#include "output.h" // Output, Screen
#include "script.h" // REG_CMD

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

extern "C" {
    int AhiInit(int);
    int AhiDevEnum(struct Asic**, DevInfo*, int asicnumber);
}

LATE_LOAD(AhiInit, "ace_ddi")
LATE_LOAD(AhiDevEnum, "ace_ddi")

static int AtiAvail() {
    return late_AhiInit && late_AhiDevEnum;
}

static void
atidbg(const char *cmd, const char *args)
{
    DevInfo devinfo;
    struct Asic *asic;
    int ret = late_AhiInit(0x0); /* 0x11, !0x100 */

    ret = late_AhiDevEnum(&asic, &devinfo, 0);

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
}
REG_CMD(AtiAvail, "ATIDBG", atidbg,
        "ATIDBG\n"
        "  Return info on ATI FB using ati dll.")
