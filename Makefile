#
# Handheld Reverse Engineering Tool
#
# This program is free software distributed under the terms of the
# license that can be found in the COPYING file.
#
# This Makefile requires GNU Make. Host operating system is always
# assumed to be Linux (or possibly other Unices), target OS can be
# either WinCE (you will need MSVC for ARM - clarm - for this) or
# Linux (you will need the cross-compiling gcc for this). The later
# is not implemented yet but should be quite handy.
#

# Target operating system (wince or linux (not implemented yet))
TARGET = wince

# Program version
VERSION=0.3.8-pre

# Output directory
OUT=out/

-include config.smak
include $(TARGET).smak

CXXFLAGS.DEF+=-DVERSION=\"$(VERSION)\"

#=============================================================== Targets ======#
vpath %.cpp src src/$(TARGET)

all: $(OUT) $(OUT)haret$E

clean:
	rm -rf $(OUT)

dep: $(wildcard src/*.cpp src/*/*.cpp)
	makedep -r $(CXXFLAGS.DEF) $(CXXFLAGS.INC) -p'$$(OUT)' -o'$$O' $^

$(OUT)haret$E: $(addprefix $(OUT),haret$O haret-res$O \
  s-cpu$O s-util$O memory$O gpio$O uart$O video$O \
  asmstuff$O irqchain$O getsetcp$O irq$O cpu-pxa$O cpu-s3c24xx$O \
  util$O output$O script$O network$O cpu$O terminal$O linboot$O bw_mem$O com_port$O lib_timing$O)
	$(LINK)

# WinCE resources (not used under Linux)
$(OUT)haret.res: include/resource.h

$(OUT):
	mkdir $@

# DO NOT DELETE this line -- makedep finds dependencies by it

$(OUT)bw_mem$O: include/xtypes.h include/bench.h include/output.h
$(OUT)cpu$O: include/xtypes.h include/cpu.h include/pxa2xx.h \
  include/s3c24xx.h include/s3c24xx/map.h include/s3c24xx/regs-dma.h \
  include/s3c24xx/regs-gpio.h include/s3c24xx/regs-serial.h \
  include/output.h include/haret.h include/memory.h include/util.h
$(OUT)gpio$O: include/xtypes.h include/gpio.h include/memory.h include/output.h
$(OUT)lib_timing$O: include/xtypes.h include/bench.h include/output.h
$(OUT)linboot$O: include/haret.h include/xtypes.h include/setup.h \
  include/memory.h include/util.h include/output.h include/gpio.h include/video.h \
  include/cpu.h include/pxa2xx.h include/s3c24xx.h include/s3c24xx/map.h \
  include/s3c24xx/regs-dma.h include/s3c24xx/regs-gpio.h \
  include/s3c24xx/regs-serial.h include/resource.h
$(OUT)memory$O: include/xtypes.h include/cpu.h include/pxa2xx.h \
  include/s3c24xx.h include/s3c24xx/map.h include/s3c24xx/regs-dma.h \
  include/s3c24xx/regs-gpio.h include/s3c24xx/regs-serial.h \
  include/memory.h include/output.h include/util.h include/haret.h
$(OUT)network$O: include/xtypes.h include/cpu.h include/pxa2xx.h \
  include/s3c24xx.h include/s3c24xx/map.h include/s3c24xx/regs-dma.h \
  include/s3c24xx/regs-gpio.h include/s3c24xx/regs-serial.h include/util.h \
  include/output.h include/terminal.h include/script.h
$(OUT)output$O: include/output.h include/util.h include/resource.h
$(OUT)script$O: include/xtypes.h include/script.h include/memory.h \
  include/video.h include/output.h include/util.h include/cpu.h \
  include/pxa2xx.h include/s3c24xx.h include/s3c24xx/map.h \
  include/s3c24xx/regs-dma.h include/s3c24xx/regs-gpio.h \
  include/s3c24xx/regs-serial.h include/gpio.h include/linboot.h include/bench.h \
  include/irq.h
$(OUT)terminal$O: include/xtypes.h include/terminal.h
$(OUT)uart$O: include/haret.h include/uart.h
$(OUT)util$O: include/util.h
$(OUT)video$O: include/xtypes.h include/video.h include/haret.h \
  include/memory.h include/output.h
$(OUT)cpu-pxa$O: include/haret.h include/xtypes.h include/setup.h \
  include/memory.h include/util.h include/output.h include/gpio.h include/video.h \
  include/cpu.h include/pxa2xx.h include/s3c24xx.h include/s3c24xx/map.h \
  include/s3c24xx/regs-dma.h include/s3c24xx/regs-gpio.h \
  include/s3c24xx/regs-serial.h include/resource.h
$(OUT)cpu-s3c24xx$O: include/haret.h include/xtypes.h include/setup.h \
  include/memory.h include/util.h include/output.h include/gpio.h include/video.h \
  include/cpu.h include/pxa2xx.h include/s3c24xx.h include/s3c24xx/map.h \
  include/s3c24xx/regs-dma.h include/s3c24xx/regs-gpio.h \
  include/s3c24xx/regs-serial.h include/uart.h include/resource.h
$(OUT)haret$O: include/xtypes.h include/resource.h include/output.h \
  include/memory.h include/script.h include/util.h include/cpu.h \
  include/pxa2xx.h include/s3c24xx.h include/s3c24xx/map.h \
  include/s3c24xx/regs-dma.h include/s3c24xx/regs-gpio.h \
  include/s3c24xx/regs-serial.h
$(OUT)s-cpu$O: include/xtypes.h include/cpu.h include/pxa2xx.h \
  include/s3c24xx.h include/s3c24xx/map.h include/s3c24xx/regs-dma.h \
  include/s3c24xx/regs-gpio.h include/s3c24xx/regs-serial.h \
  include/output.h include/haret.h
