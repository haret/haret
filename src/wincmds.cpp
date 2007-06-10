/* Commands that interface with Windows CE internals.
 *
 * (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
 *
 * For conditions of use see file COPYING
 */

#include <windows.h> // Sleep
#include <time.h> // time
#include "pwinuser.h" // NLedSetDevice
#include "nled.h" // NLED_SETTINGS_INFO

#include "xtypes.h" // uint32
#include "output.h" // Output
#include "script.h" // REG_CMD
#include "lateload.h" // LATE_LOAD

static void
cmd_sleep(const char *cmd, const char *args)
{
    uint32 msec;
    if (!get_expression(&args, &msec)) {
        ScriptError("Expected <milliseconds>");
        return;
    }
    Sleep(msec);
}
REG_CMD(0, "S|LEEP", cmd_sleep,
        "SLEEP <milliseconds>\n"
        "  Sleep for given amount of milliseconds.")

LATE_LOAD(LoadLibraryExW, "coredll")

static int LLXAvail() {
    return !!late_LoadLibraryExW;
}

static void
cmd_LoadLibraryEx(const char *cmd, const char *args)
{
    char name[MAX_CMDLEN];
    if (get_token(&args, name, sizeof(name))) {
        ScriptError("Expected <file name>");
        return;
    }
    wchar_t wname[MAX_CMDLEN];
    mbstowcs(wname, name, ARRAY_SIZE(wname));

    Output("Calling LoadLibraryEx on '%ls'", wname);
    HMODULE hMod = late_LoadLibraryExW(wname, 0, LOAD_LIBRARY_AS_DATAFILE);
    Output("Call returned %p", hMod);
}
REG_CMD(LLXAvail, "LOADLIBRARYEX", cmd_LoadLibraryEx,
        "LOADLIBRARYEX <file name>\n"
        "  Call LoadLibraryEx on the specified file and print the handle.")

static void
LedSet(const char *cmd, const char *args)
{
    uint32 id, value;
    if (!get_expression(&args, &id) || !get_expression(&args, &value)) {
        ScriptError("Expected <id> <value>");
        return;
    }
    NLED_SETTINGS_INFO settings;
    settings.LedNum = id;
    settings.OffOnBlink = value;
    int ret = NLedSetDevice(NLED_SETTINGS_INFO_ID, &settings);
    if (!ret)
        Output("Return status of NLedSetDevice indicates failure");
}
REG_CMD(0, "NLEDSET", LedSet,
        "NLEDSET <id> <value>\n"
        "  Call NLedSetDevice to alter an LED setting.\n"
        "  <id> is the LED id.\n"
        "  <value> may be 0 for off, 1 for on, or 2 for blink.")

// The GetSystemPowerStatusEx2 is only available on wince 2.12 and
// later.  However, earlier versions of CE have GetSystemPowerStatusEx
// which uses a struct that is compatible (but smaller) than the
// original.
DWORD alt_GetSystemPowerStatusEx2(
    PSYSTEM_POWER_STATUS_EX2 pSystemPowerStatusEx2, DWORD dwLen, BOOL fUpdate)
{
    bool ret = GetSystemPowerStatusEx(
        (SYSTEM_POWER_STATUS_EX *)pSystemPowerStatusEx2, fUpdate);
    if (ret)
        return sizeof(SYSTEM_POWER_STATUS_EX);
    return 0;
}

LATE_LOAD_ALT(GetSystemPowerStatusEx2, "coredll")

static void
powerMon(const char *cmd, const char *args)
{
    uint32 seconds;
    if (!get_expression(&args, &seconds))
        seconds = 0;

    uint32 start_time = GetTickCount();
    uint32 cur_time = start_time;
    uint32 fin_time = cur_time + seconds * 1000;

    for (;;) {
        SYSTEM_POWER_STATUS_EX2 stat2;
        memset(&stat2, 0, sizeof(stat2));
        int ret = late_GetSystemPowerStatusEx2(&stat2, sizeof(stat2), false);
        if (!ret) {
            Output(C_INFO "GetSystemPowerStatusEx2");
            return;
        }

        Output("%06d: %5d %5d %% %5ld %5ld %5ld %5ld "
               "%5d %5d %5ld %5ld %5d %5d"
               "  %5d %5d %5ld %5ld %5ld %5ld %5ld %5d",
               cur_time - start_time,                  //
               stat2.BatteryFlag,                      //
               stat2.BatteryLifePercent,               //
               stat2.BatteryVoltage,                   //  
               stat2.BatteryCurrent,                   //  
               stat2.BatteryAverageCurrent,            //  
               stat2.BatteryTemperature,               //  
               stat2.ACLineStatus,                     //  0x00
               stat2.Reserved1,                        //  0x00
               stat2.BatteryLifeTime,                  //  -1
               stat2.BatteryFullLifeTime,              //  -1
               stat2.Reserved2,                        //  0x00
               stat2.BackupBatteryFlag,                //  0x01
               stat2.BackupBatteryLifePercent,         //  255
               stat2.Reserved3,                        //  0x00
               stat2.BackupBatteryLifeTime,            //  -1
               stat2.BackupBatteryFullLifeTime,        //  -1
               stat2.BatteryAverageInterval,           //  0
               stat2.BatterymAHourConsumed,            //  0
               stat2.BackupBatteryVoltage,             //  0
               stat2.BatteryChemistry);                 //  5

        cur_time = GetTickCount();
        if (cur_time >= fin_time)
            break;
        Sleep(1000);
    }
}
REG_CMD(0, "POWERMON", powerMon,
        "POWERMON [<seconds>]\n"
        "  Watch power status")

static void
playSound(const char *cmd, const char *args)
{
    uint32 seconds;
    if (!get_expression(&args, &seconds))
        seconds = 0;

    Output("Playing chord.wav for %d seconds",seconds);
    int ret=PlaySound(L"\\Windows\\chord.wav", 0, SND_LOOP|SND_ASYNC|SND_FILENAME);
    Sleep(seconds*1000);
    if (ret)
        PlaySound(0, 0, 0);
}
REG_CMD(0, "PLAYSOUND", playSound,
        "PLAYSOUND [<seconds>]\n"
        "  Plays chord.wav")

// From MSDN docs.
#define SETPOWERMANAGEMENT 6147
typedef struct VIDEOPOWER_MANAGEMENT {
    ULONG Length;
    ULONG DPMSVersion;
    ULONG PowerState;
} VIDEO_POWER_MANAGEMENT, *PVIDEO_POWER_MANAGEMENT;

static void
LCDonoff(const char *cmd, const char *args)
{
    uint32 state;
    if (!get_expression(&args, &state)) {
        ScriptError("Expected <state>");
        return;
    }

    HDC gdc = GetDC(NULL);

    Output("LcdPower(%d) dc=%p", state, gdc);

    VIDEO_POWER_MANAGEMENT pm;
    pm.Length = sizeof(pm);
    pm.PowerState = state;
    int ret=ExtEscape(gdc, SETPOWERMANAGEMENT, pm.Length, (LPCSTR)&pm, 0, NULL);
    ReleaseDC(NULL, gdc);

    if (ret < 0)
        Output("ExtEscape==%d (code %ld)", ret, GetLastError());
    else
        Output("ExtEscape==%d", ret);
}
REG_CMD(0, "SETLCD", LCDonoff,
        "SETLCD <state>\n"
        "  Set the LCD power start (1=on, 2=standby, 3=suspend, 4=off)")
