/* Commands that interface with Windows CE internals.
 *
 * (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
 *
 * For conditions of use see file COPYING
 */

#include <windows.h> // Sleep
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
