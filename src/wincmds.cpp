/* Commands that interface with Windows CE internals.
 *
 * (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
 *
 * For conditions of use see file COPYING
 */

#include <windows.h> // Sleep

#include "xtypes.h" // uint32
#include "output.h" // Complain
#include "script.h" // REG_CMD

static void
cmd_sleep(const char *cmd, const char *x)
{
    uint32 msec;
    if (!get_expression(&x, &msec)) {
        Complain(C_ERROR("line %d: Expected <milliseconds>"), ScriptLine);
        return;
    }
    Sleep(msec);
}
REG_CMD(0, "S|LEEP", cmd_sleep,
        "SLEEP <milliseconds>\n"
        "  Sleep for given amount of milliseconds.")
