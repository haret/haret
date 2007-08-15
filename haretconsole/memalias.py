# Utility to parse out memory dumps from haret and translate them into
# register names.
import re

# Regs_xxx = {addr: name, ...}
#   or
# Regs_xxx = {addr: (name, ((bits, name), (bits,name), ...)) }
#   or
# Regs_xxx = {addr: (name, func(bit)) }

RegsList = {}


######################################################################
# PXA
######################################################################

irqs1 = (
    (0, "SSP3"), (1, "MSL"), (2, "USBh2"), (3, "USBh1"),
    (4, "Keypad"), (5, "MemoryStick"), (6, "pI2C"), (7, "OS Timer"),
    (8, "GPIO0"), (9, "GPIO1"), (10, "GPIOx"), (11, "USBc"),
    (12, "PMU"), (13, "I2S"), (14, "AC97"), (15, "USIM"),
    (16, "SSP2"), (17, "LCD"), (18, "I2C"), (19, "ICP"),
    (20, "STUART"), (21, "BTUART"), (22, "FFUART"), (23, "MMC"),
    (24, "SSP"), (25, "DMA"), (26, "TMR0"), (27, "TMR1"),
    (28, "TMR2"), (29, "TMR3"), (30, "RTC0"), (31, "RTC1"))
irqs2 = ((0, "TPM"), (1, "QCap"))

Regs_pxa27x = {
    0x40D00000: ("ICIP", irqs1), 0x40D0009C: ("ICIP2", irqs2),
    0x40E00048: ("GEDR0", (lambda bit: "GPIO%d" % bit)),
    0x40E0004c: ("GEDR1", (lambda bit: "GPIO%d" % (bit+32))),
    0x40E00050: ("GEDR2", (lambda bit: "GPIO%d" % (bit+64))),
    0x40E00148: ("GEDR3", (lambda bit: "GPIO%d" % (bit+96))),

    0x40E00000: ("GPLR0", (lambda bit: "GPIO%d" % bit)),
    0x40E00004: ("GPLR1", (lambda bit: "GPIO%d" % (bit+32))),
    0x40E00008: ("GPLR2", (lambda bit: "GPIO%d" % (bit+64))),
    0x40E00100: ("GPLR3", (lambda bit: "GPIO%d" % (bit+96))),

    0x40E0000C: ("GPDR0", (lambda bit: "GPIO%d" % bit)),
    0x40E00010: ("GPDR1", (lambda bit: "GPIO%d" % (bit+32))),
    0x40E00014: ("GPDR2", (lambda bit: "GPIO%d" % (bit+64))),
    0x40E0010C: ("GPDR3", (lambda bit: "GPIO%d" % (bit+96))),
    }
RegsList['ARCH:PXA27x'] = Regs_pxa27x

# HTC Apache specific registers
Regs_Apache = Regs_pxa27x.copy()
Regs_Apache.update({
    0x0a000000: ("cpldirq", (lambda bit: "CPLD%d" % bit)),
    })
RegsList['Apache'] = Regs_Apache

# PXA 26x registers
irqs1 = (
    (7, "HW uart"),
    (8, "GPIO0"), (9, "GPIO1"), (10, "GPIOx"), (11, "USB"),
    (12, "PMU"), (13, "I2S"), (14, "AC97"), (15, "aSSP"),
    (16, "nSSP"), (17, "LCD"), (18, "I2C"), (19, "ICP"),
    (20, "STUART"), (21, "BTUART"), (22, "FFUART"), (23, "MMC"),
    (24, "SSP"), (25, "DMA"), (26, "TMR0"), (27, "TMR1"),
    (28, "TMR2"), (29, "TMR3"), (30, "RTC0"), (31, "RTC1"))
Regs_pxa = Regs_pxa27x.copy()
Regs_pxa.update({
    0x40D00000: ("ICIP", irqs1)
    })
RegsList['ARCH:PXA'] = Regs_pxa


######################################################################
# s3c24xx
######################################################################

irqs = (
    (31, "INT_ADC"), (30, "INT_RTC"), (29, "INT_SPI1"), (28, "INT_UART0"),
    (27, "INT_IIC"), (26, "INT_USBH"), (25, "INT_USBD"), (24, "INT_NFCON"),
    (23, "INT_UART1"), (22, "INT_SPI0"), (21, "INT_SDI"), (20, "INT_DMA3"),
    (19, "INT_DMA2"), (18, "INT_DMA1"), (17, "INT_DMA0"), (16, "INT_LCD"),
    (15, "INT_UART2"), (14, "INT_TIMER4"),
    (13, "INT_TIMER3"), (12, "INT_TIMER2"),
    (11, "INT_TIMER1"), (10, "INT_TIMER0"), (9, "INT_WDT"), (8, "INT_TICK"),
    (7, "nBATT_FLT"), (6, "INT_CAM"), (5, "EINT8_23"), (4, "EINT4_7"),
    (3, "EINT3"), (2, "EINT2"), (1, "EINT1"), (0, "EINT0"))

Regs_s3c2442 = {
    0x4A000010: ("INTPND", irqs),
    0x56000004: ("GPADAT", (lambda bit: "GPA%d" % bit)),
    0x56000014: ("GPBDAT", (lambda bit: "GPB%d" % bit)),
    0x56000024: ("GPCDAT", (lambda bit: "GPC%d" % bit)),
    0x56000034: ("GPDDAT", (lambda bit: "GPD%d" % bit)),
    0x56000044: ("GPEDAT", (lambda bit: "GPE%d" % bit)),
    0x56000054: ("GPFDAT", (lambda bit: "GPF%d" % bit)),
    0x56000064: ("GPGDAT", (lambda bit: "GPG%d" % bit)),
    0x56000074: ("GPHDAT", (lambda bit: "GPH%d" % bit)),
    0x560000d4: ("GPJDAT", (lambda bit: "GPJ%d" % bit)),
    0x56000080: ("MISCCR", (("22-20", "BATT_FUNC"),
                            (19, "OFFREFRESH"),
                            (18, "nEN_SCLK1"))),
    0x560000a8: ("EINTPEND", (lambda bit: "EINT%d" % bit)),
    }
RegsList['ARCH:s3c2442'] = Regs_s3c2442

# HTC Hermes specific registers
Regs_Hermes = Regs_s3c2442.copy()
Regs_Hermes.update({
    0x08000004: ("cpldirq", (lambda bit: "CPLD%d" % bit)),
    })
RegsList['Hermes'] = Regs_Hermes


######################################################################
# omap850
######################################################################

Regs_omap850 = {
    0xfffecb00: ("IH1", (lambda bit: "IH1-%d" % bit)),
    0xfffe0000: ("IH2", (lambda bit: "IH2-%d" % bit)),
    0xfffe0100: ("IH2", (lambda bit: "IH2-%d" % (bit + 32))),
    0xfffbc000: ("GPIO0", (lambda bit: "GPIO%d" % bit)),
    0xfffbc800: ("GPIO1", (lambda bit: "GPIO%d" % (bit + 32))),
    0xfffbd000: ("GPIO2", (lambda bit: "GPIO%d" % (bit + 64))),
    0xfffbd800: ("GPIO3", (lambda bit: "GPIO%d" % (bit + 96))),
    0xfffbe000: ("GPIO4", (lambda bit: "GPIO%d" % (bit + 128))),
    0xfffbe800: ("GPIO5", (lambda bit: "GPIO%d" % (bit + 160))),
    }
RegsList['ARCH:OMAP850'] = Regs_omap850


######################################################################
# Register list pre-processing
######################################################################

# Parse a bit name description.  It can accept a function that
# converts a number to a name or a list of bit/name 2-tuples that
# describes a bit.  The description can be an integer bit number or a
# string description of a set of bits (a comma separated list and/or
# hyphen separated range).
def parsebits(defs):
    if type(defs) != type(()):
        # lambda
        return tuple([((1<<i), defs(i)) for i in range(32)])
    out = ()
    for desc, val in defs:
        if type(desc) == type(1):
            out += (((1<<desc),val),)
            continue
        bits = ()
        for bit in desc.split(','):
            bitrange = bit.split('-', 1)
            if len(bitrange) > 1:
                bits += tuple(range(int(bitrange[0]), int(bitrange[1])-1, -1))
            else:
                bits += (int(bit),)
        mask = 0
        for bit in bits:
            mask |= (1<<bit)
        out += ((mask, val),)
    return out

# Internal function called at script startup time.  It processes the
# register maps into an internal uniform format.  The format is:
# RegsList = {"archname": {paddr: ("regname", ((bit_n, "bitname"),...))}}
def preproc():
    global RegsList
    rl = {}
    for name, regs in RegsList.items():
        archinfo = {}
        for addr, info in regs.items():
            if type(info) == type(""):
                info = (info,)
            else:
                info = (info[0], parsebits(info[1]))
            archinfo[addr] = info
        rl[name] = archinfo
    RegsList = rl
preproc()

######################################################################
# Memory display
######################################################################

# Last process "clock" val - used for determining clock offsets
LastClock = 0

# Given a matching TIMEPRE_S regex - return the string description of it.
def getClock(m):
    global LastClock
    clock = m.group('clock')
    t = "%07.3f" % (int(m.group('time')) / 1000.0,)
    if clock is None:
        return t
    clock = int(clock, 16)
    out = "%07d" % (clock - LastClock,)
    LastClock = clock
    return "%s(%s)" % (t, out)

# Internal class that stores an active "watch" region.  It is used
# when reformating the output string of a memory trace event.
class memDisplay:
    def __init__(self, reginfo, type, pos):
        self.type = ""
        if type:
            self.type = " " + type
        self.pos = pos
        self.name = reginfo[0]
        self.bits = reginfo[1]
    # Return a string representation of a set of changed bits.
    def getValue(self, desc, bits, mask, val):
        count = 0
        outval = 0
        pos = []
        bits &= mask
        for i in range(32):
            bit = 1<<i
            if bit & bits:
                if bit & val:
                    outval |= 1<<count
                pos.append(i)
                count += 1
        ignorelist = " ".join(["%d" % (self.pos*32+i) for i in pos])
        return " %s(%s)=%d" % (desc, ignorelist, outval)
    # Main function call into this class - display a mem trace line
    def displayFunc(self, m):
        val = int(m.group('val'), 16)
        mask = int(m.group('mask'), 16)
        if not self.bits or not mask:
            print "%s%s %8s=%08x (%08x)" % (
                getClock(m), self.type, self.name, val, mask)
        origmask = mask
        out = ""
        for bits, desc in self.bits:
            if bits & mask:
                out += self.getValue(desc, bits, mask, val)
                mask &= ~bits
        if mask:
            print "%s%s %8s=%08x (%08x)%s" % (
                getClock(m), self.type, self.name, val, origmask, out)
        else:
            print "%s%s %8s:%s" % (
                getClock(m), self.type, self.name, out)

# Default memory tracing line if no custom register specified.
def defaultFunc(m):
    e = m.end('clock')
    if e < 0:
        e = m.end('time')
    print getClock(m), m.string[e+6:]


######################################################################
# Parsing
######################################################################

# Regular expressions to search for.
TIMEPRE_S = r'^(?P<time>[0-9]+): ((?P<clock>[0-9a-f]+): )?'
re_detect = re.compile(r"^Detected machine (?P<name>.*)/(?P<arch>.*)"
                       r" \(Plat=.*\)$")
re_begin = re.compile(r"^Beginning memory tracing\.$")
re_watch = re.compile(r"^Watching (?P<type>.*)\((?P<pos>\d+)\):"
                      r" Addr (?P<vaddr>.*)\(@(?P<paddr>.*)\)$")
re_mem = re.compile(TIMEPRE_S + r"mem (?P<vaddr>.*)=(?P<val>.*)"
                    r" \((?P<mask>.*)\)$")

# Storage of known named registers that are being "watched"
VirtMap = {}
# The active architecture named registers
ArchRegs = {}

def handleWatch(m):
    vaddr = m.group('vaddr')
    paddr = int(m.group('paddr'), 16)
    reginfo = ArchRegs.get(paddr)
    if reginfo is not None:
        md = memDisplay(reginfo, m.group('type'), int(m.group('pos')))
        VirtMap[vaddr] = md.displayFunc
    print m.string

def handleBegin(m):
    global LastClock
    LastClock = 0
    VirtMap.clear()
    print m.string

def handleDetect(m):
    global ArchRegs
    ar = RegsList.get(m.group('name'))
    if ar is None:
        ar = RegsList.get("ARCH:"+m.group('arch'), {})
    ArchRegs = ar
    print m.string

def procline(line):
    m = re_mem.match(line)
    if m:
        func = VirtMap.get(m.group('vaddr'), defaultFunc)
        func(m)
        return
    m = re_watch.match(line)
    if m:
        return handleWatch(m)
    m = re_begin.match(line)
    if m:
        return handleBegin(m)
    m = re_detect.match(line)
    if m:
        return handleDetect(m)
    print line.rstrip()
