/* Commands to fiddle with the MSM RPCCLK via the clkregim dll.
 *
 *
 * For conditions of use see file COPYING
 */

#include "lateload.h" // LATE_LOAD
#include "output.h" // Output, Screen
#include "script.h" // REG_CMD
#include <math.h>

/****************************************************************
 * Dump clock frequencies using clkregim.dll
 ****************************************************************/

extern "C" {
    int clk_regime_msm_get_clk_freq_khz(int);
}

LATE_LOAD(clk_regime_msm_get_clk_freq_khz, "clkregim")

static int ClkRegimeAvail() {
    return !!late_clk_regime_msm_get_clk_freq_khz;
}

static void
msm_clk_dbg_dump(const char *cmd, const char *args)
{
    uint32 start, end, clk1, clk2;

    // Make sure we got the parameters we need
    if (!get_expression(&args, &start) ||
        !get_expression(&args, &end))
    {
        ScriptError("Expected <regime_id_start> <regime_id_end>");
        return;
    }
    
    // Monkey-proof
    if (start > end)
    {
        ScriptError("<regime_id_end> must be equal or larger than <regime_id_start>");
        return;
    }
    
    // Nice header
    Output("     regime     KHz       |     regime     KHz");
    Output("--------------------------+-----------------------");

    // We want to display the clocks like this:
    // 0x1 | 0x3
    // 0x2 | 0x4
    int rows = ((end - start) / 2) + 1;
    for (int i=0; i < rows; i++)
    {
        clk1 = start;
        clk2 = start + rows;

        // Show one or two columns? (One column only happens when this is the last row
        // and an uneven number of clocks was dumped.
        if (clk2 > end)
        {
            Output("  %3d / 0x%02x = %-8d   |",
                clk1, clk1, late_clk_regime_msm_get_clk_freq_khz(clk1));
        }
        else
        {
            Output("  %3d / 0x%02x = %-8d   |  %3d / 0x%02x = %-8d",
                clk1, clk1, late_clk_regime_msm_get_clk_freq_khz(clk1),
                clk2, clk2, late_clk_regime_msm_get_clk_freq_khz(clk2));
        }

        start++;
    }
}

REG_DUMP(ClkRegimeAvail, "MSMCLKKHZ", msm_clk_dbg_dump,
         "MSMCLKKHZ <regime_id_start> <regime_id_end>\n"
         "  Dump MSM clock frequencies using clkregim.dll.")

