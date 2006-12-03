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
#include "output.h" // Complain
#include "script.h" // REG_CMD
#include "lateload.h" // LATE_LOAD

static void
cmd_sleep(const char *cmd, const char *args)
{
    uint32 msec;
    if (!get_expression(&args, &msec)) {
        Complain(C_ERROR("line %d: Expected <milliseconds>"), ScriptLine);
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
    char *name = get_token(&args);
    if (!name) {
        Complain(C_ERROR("line %d: Expected <file name>"), ScriptLine);
        return;
    }
    wchar_t wname[200];
    MultiByteToWideChar(CP_ACP, 0, name, -1, wname, sizeof(wname));

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
        Complain(C_ERROR("line %d: Expected <id> <value>"), ScriptLine);
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

LATE_LOAD(GetSystemPowerStatusEx2, "coredll")

static void
powerMon(const char *cmd, const char *args)
{
    uint32 seconds;
    if (!get_expression(&args, &seconds))
        seconds = 0;

    int cur_time = time(NULL);
    int fin_time = cur_time + seconds;

    for (;;) {
        if (late_GetSystemPowerStatusEx2) {
            SYSTEM_POWER_STATUS_EX2 stat2;
            int ret = late_GetSystemPowerStatusEx2(&stat2, sizeof(stat2), false);
            if (!ret) {
                Complain(L"GetSystemPowerStatusEx2");
                return;
            }

            Output("%5ld %5d %5d %% %5ld %5ld %5ld %5ld "
                   "%5d %5d %5ld %5ld %5d %5d"
                   "  %5d %5d %5ld %5ld %5ld %5ld %5ld %5d",
                   GetTickCount(),                         //
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
        } else {
            // Use older call (it has less info).
            SYSTEM_POWER_STATUS_EX stat;
            int ret = GetSystemPowerStatusEx(&stat, false);
            if (!ret) {
                Complain(L"GetSystemPowerStatusEx");
                return;
            }

            Output("%5ld %5d %5d %% %5d %5d %5d %5d "
                   "%5d %5d %5ld %5ld %5d %5d"
                   "  %5d %5d %5ld %5ld %5d %5d %5d %5d",
                   GetTickCount(),                         //
                   stat.BatteryFlag,                      //
                   stat.BatteryLifePercent,               //
                   stat.ACLineStatus,                     //  0x00
                   -1,-1,-1,-1,
                   stat.Reserved1,                        //  0x00
                   stat.BatteryLifeTime,                  //  -1
                   stat.BatteryFullLifeTime,              //  -1
                   stat.Reserved2,                        //  0x00
                   stat.BackupBatteryFlag,                //  0x01
                   stat.BackupBatteryLifePercent,         //  255
                   stat.Reserved3,                        //  0x00
                   stat.BackupBatteryLifeTime,            //  -1
                   stat.BackupBatteryFullLifeTime,        //  -1
                   -1,-1,-1,-1);
        }

        cur_time = time(NULL);
        if (cur_time >= fin_time)
            break;
        Sleep(1000);
    }
}
REG_CMD(0, "POWERMON", powerMon,
        "POWERMON [<seconds>]\n"
        "  Watch power status")
