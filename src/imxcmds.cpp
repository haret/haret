/*
    Commands specific to the i.MX21 processor
    Copyright (C) 2009 Holger Schurig

    For conditions of use see file COPYING
*/

#include "memory.h" // memPhysMap
#include "output.h" // Output
#include "script.h" // REG_xxx
#include "arch-imx.h" // testIMX



/*
 * cmd_dump_eim() produces an output like this:
 *
 *                   BCD BCS            DOL
 * nCS   addr  SP WP DCT RWA PSZ PME SY RWN CNC  WSC EW WWS EDC
 * 0 c8000000  0  0   0   0   0   0  0   0   0   62  0   0   0
 * 1 cc000000  0  0   0   0   0   0  0   0   0   12  0   0   0
 * 2 d0000000  0  0   0   0   0   0  0   0   0    0  0   0   0
 * 3 d1000000  0  0   0   0   0   0  0   0   0    0  0   0   0
 * 4 d2000000  0  0   0   0   0   0  0   0   0    0  0   0   0
 * 5 d3000000  0  0   0   0   0   0  0   0   0   63  0   0   0
 *
 * nCS   addr OEA OEN WEA WEN CSA EBC     DSZ CSN PSR CRE WRP EN
 * 0 c8000000   2   0   0   0   0   1  [31:0]   0   0   0   0  1
 * 1 cc000000   5   1   4   1   0   1 [23:16]   0   0   0   0  1
 * 2 d0000000   0   0   0   0   0   0 [31:24]   0   0   0   0  0
 * 3 d1000000   0   0   0   0   0   0 [31:24]   0   0   0   0  0
 * 4 d2000000   0   0   0   0   0   0 [31:24]   0   0   0   0  0
 * 5 d3000000   0   0   0   0   0   0 [31:16]   0   0   0   0  1
 *
 * This gives an overview over which external memory areas ("addr")
 * are enabled ("EN") and use which bus-width ("DSZ"). For the
 * rest of the column-meanings, refer to the i.MX21 Reference
 * Manual, Chapter 20 "External Interface Module (EIM)".
 */

static void
cmd_dump_eim(const char *cmd, const char *args)
{
	uint32 base = 0xdf001000;
	int cs;

	uint32 addr[6] = {
		0xc8000000,
		0xcc000000,
		0xd0000000,
		0xd1000000,
		0xd2000000,
		0xd3000000
	};

	Output("                  BCD BCS            DOL");
	Output("nCS   addr  SP WP DCT RWA PSZ PME SY RWN CNC  WSC EW WWS EDC");
	for (cs=0; cs<=5; cs++) {
		uint32 u = memPhysRead(base + cs*8);
		Output("%d %08x "
			"%2d %2d %3d %3d "
			"%3d %3d %2d %3d "
			"%3d %4d %2d %3d %3d",
			cs, addr[cs],
			(u >> 31) &    1, // SP
			(u >> 30) &    1, // WP
			(u >> 28) &    3, // BCD/DCT
			(u >> 24) &  0xf, // BCS/RWA

			(u >> 22) &    3, // PSZ
			(u >> 21) &    1, // PME
			(u >> 20) &    1, // SYNC
			(u >> 16) &  0xf, // DOL/RWN

			(u >> 14) &    3, // CNC
			(u >>  8) & 0x3f, // WSC
			(u >>  7) &    1, // EW
			(u >>  4) &    7, // WWS

			u         & 0xf); // EDC
	}
	char* dsz[8] = {
		"[31:24]", "[23:16]", "[15:8]", "[7:0]", // 8-bit modes
		"[31:16]", "[15:0]", // 16-bit modes
		"[31:0]", // 32-bit modes
		"resrvd", // Reserved
	};

	Output("\r\nnCS   addr OEA OEN WEA WEN CSA EBC     DSZ CSN PSR CRE WRP EN");
	for (cs=0; cs<=5; cs++) {
		uint32 l = memPhysRead(base + cs*8 + 4);
		Output("%d %08x "
			"%3d %3d %3d %3d "
			"%3d %3d %7s %3d "
			"%3d %3d %3d %2d",
			cs, addr[cs],
			(l >> 28) & 0xf, // OEA
			(l >> 24) & 0xf, // OEN
			(l >> 20) & 0xf, // WEA
			(l >> 16) & 0xf, // WEN

			(l >> 12) & 0xf, // CSA
			(l >> 11) &   1, // EBC
			dsz[(l >> 8) & 7], // DSZ
			(l >>  4) & 0xf, // CSN

			(l >>  3) &   1, // PSR
			(l >>  2) &   1, // CRE
			(l >>  1) &   1, // WRAP
			l         &   1); // CSEN
	}
}

REG_DUMP(testIMX, "EIM", cmd_dump_eim,
         "EIM\n"
         "  Display the configuration of the EIM (External Interface Module)")


/*
 * The i.MX21 has 6 blocks with 32 GPIOs each. So says the manual,
 * but that is not true, e.g. GPIO A3 doesn't go to any pin :-)
 * The pins have a quite complex logic with a "IOMUX" and "GPIO"
 * block, that allow any pin to be connected the GPIO registers
 * or to internal function lines ("A_IN", "B_IN", "C_IN", "A_OUT"
 * and "B_OUT").
 *
 * I think it's overly complex to find out if a pin has this or
 * that function ... therefore the extensive dump function here.
 *
 *
 * For more info, refer to chapter 15 "General-Purpose I/O (GPIO)"
 * as well as to chapter 2.2 "I/O Power Supply and Signal
 * Multiplexing Scheme". In the example below, you've to look
 * into chapter 2.2 to find out what the primary function for
 * pin "PE 9" is.
 *
 * An abbreviated sample output of "DUMP EIM" looks like this:
 *
 * Pin Function GIUS  GPR DDIR  OCR A_OUT  DR SSR  ICR Int PUEN
 * ...
 * PE 9 primary  mux prim   in   DR   one  0   1 rise dis    1
 * PE10  GPIO_I GPIO prim   in B_IN   one  0   0 rise ENA    1
 * PE11  GPIO_O GPIO  ALT  OUT B_IN   one  0   0 rise dis    0
 *
 *
 *     DDIR     OCR1     OCR2     ICONFA1  ICONFA2  DR
 * PFA 80000000 00000000 c0000000 ffffffff ffffffff 80000000
 * PFB 000003a0 000fcc00 00000000 ffffffff ffffffff 00000190
 * PFC 01104000 30000000 00030300 fffcffff ffffffff 01100000
 * PFD 06000000 00000000 003c0000 ffffffff ffffffff 14000000
 * PFE 00030806 00c00c3c 0000000f ffffffff ffffffff 00030020
 * PFF 00010000 00000000 00000003 ffffffff fffff3ff 00010000
 *
 *     GIUS     SSR      ICR1     ICR2     IMASK    GPR      PUEN
 * PFA 00000000 a3c30c00 00000000 00000000 00000000 00000000 ffffffff
 * PFB fe8003f0 2fd10190 00000000 04340000 26000000 000003a0 cffffcdf
 * PFC 0ff0c100 17fbab60 00000000 00040000 02000000 01104000 feef811f
 * PFD 5ff80000 75360000 00000000 3001a780 49f80000 06000000 f1ffffff
 * PFE 0003cc06 007ff2e0 00200000 00000000 00000400 00030806 ff3cb7f9
 * PFF 00210000 00617fe7 00000000 00000000 00000000 00010000 fffeffff
 *
 */

static void
cmd_dump_gpio(const char *cmd, const char *args)
{
	char* ocr[4]   = {"A_IN",  "B_IN", "C_IN", "DR" };
	char* iconf[4] = {"pin",   "ISR",  "zero", "one" };
	char* icr[4]   = {"rise",  "fall", "high", "low"};

	uint32 store[6][15];

	int fb;
	for (fb=0; fb<6; fb++) {
		uint32 base    = 0x10015000 + 0x100*fb;
		uint32 ddir    = store[fb][ 0] = memPhysRead(base);
		uint32 ocr1    = store[fb][ 1] = memPhysRead(base+4);
		uint32 ocr2    = store[fb][ 2] = memPhysRead(base+8);
		uint32 iconfa1 = store[fb][ 3] = memPhysRead(base+0x0c);
		uint32 iconfa2 = store[fb][ 4] = memPhysRead(base+0x10);
		// The i.MX21 has the ICONFB registers, but there's no GPIO
		// with a B_OUT function according to chapter 2.2 of the
		// reference manual
		//uint32 iconfb1 = store[fb][ 5] = memPhysRead(base+0x14);
		//uint32 iconfb2 = store[fb][ 6] = memPhysRead(base+0x18);
		uint32 dr      = store[fb][ 7] = memPhysRead(base+0x1c);
		uint32 gius    = store[fb][ 8] = memPhysRead(base+0x20);
		uint32 ssr     = store[fb][ 9] = memPhysRead(base+0x24);
		uint32 icr1    = store[fb][10] = memPhysRead(base+0x28);
		uint32 icr2    = store[fb][11] = memPhysRead(base+0x2c);
		uint32 imask   = store[fb][12] = memPhysRead(base+0x30);
		uint32 gpr     = store[fb][13] = memPhysRead(base+0x38);
		uint32 puen    = store[fb][14] = memPhysRead(base+0x40);

		Output("\r\nPin Function GIUS  GPR DDIR  OCR A_OUT  DR SSR ICR Int PUEN");
		for (int gpio=0; gpio<=31; gpio++) {

			// The following code more or less decodes the IOMUX part
			const char *function;
			int is_out  = (ddir >> gpio) & 1;
			int is_gpio = (gius >> gpio) & 1;
			int is_alt  = (gpr  >> gpio) & 1;
			if (is_gpio)
				function = is_out ? "GPIO_O" : "GPIO_I";
			else {
				function = is_alt ? "alt" : "primary";
			}

			Output("P%c%2d %7s "
				"%4s %4s %4s "
				"%4s %5s "
				"%2d %3d %4s "
				"%3s %4d",

				'A'+fb, gpio, function,

				is_gpio ? "GPIO" : "mux",
				is_alt ? "ALT" : "prim",
				is_out  ? "OUT"  : "in",

				ocr[(gpio < 16 ? ocr1 : ocr2) >> (gpio>>1) & 3],
				iconf[(gpio < 16 ? iconfa1 : iconfa2) >> (gpio>>1) & 3],
				//iconf[(gpio < 16 ? iconfb1 : iconfb2) >> (gpio>>1) & 3],

				(dr >> gpio) & 1,
				(ssr >> gpio) & 1,
				icr[(gpio < 16 ? icr1 : icr2) >> (gpio>>1) & 3],

				(imask >> gpio) & 1 ? "ENA" : "dis",
				(puen >> gpio) & 1
				);
		}
	}

	Output("\r\n    DDIR     OCR1     OCR2     ICONFA1  ICONFA2  DR");
	for (fb=0; fb<6; fb++) {
		Output("PF%c %08x %08x %08x %08x %08x %08x",
			'A'+fb,
			store[fb][0], store[fb][1], store[fb][2], store[fb][3],
			store[fb][4], store[fb][7]);
	}
	Output("\r\n    GIUS     SSR      ICR1     ICR2     IMASK    GPR      PUEN");
	for (fb=0; fb<6; fb++) {
		Output("PF%c %08x %08x %08x %08x %08x %08x %08x",
			'A'+fb,
			store[fb][8], store[fb][9], store[fb][10], store[fb][11],
			store[fb][12], store[fb][13], store[fb][14]);
	}
}

REG_DUMP(testIMX, "GPIO", cmd_dump_gpio,
         "GPIO\n"
         "  Display the configuration of the GPIOs")
