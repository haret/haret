#
# Handheld Reverse Engineering Tool
#
# This program is free software distributed under the terms of the
# license that can be found in the COPYING file.
#
# This Makefile requires GNU Make.
#

# Program version
VERSION=0.4.8
#VERSION=pre-0.4.8-$(shell date +"%Y%m%d_%H%M%S")

# Output directory
OUT=out/

# Default compiler flags (note -march=armv4 is needed for 16 bit insns)
CXXFLAGS = -Wall -O -g -MD -march=armv4 -Iinclude
LDFLAGS = -Wl,--major-subsystem-version=2,--minor-subsystem-version=10
# LDFLAGS to debug invalid imports in exe
#LDFLAGS = -Wl,-M -Wl,--cref

LIBS = -lwinsock

all: $(OUT) $(OUT)haret.exe

# Run with "make V=1" to see the actual compile commands
ifdef V
Q=
else
Q=@
endif

.PHONY : all FORCE

vpath %.cpp src src/wince src/mach
vpath %.S src src/wince
vpath %.rc src/wince

################ cegcc settings

BASE ?= /opt/mingw32ce
export BASE

RC = $(BASE)/bin/arm-wince-mingw32ce-windres
RCFLAGS = -r -l 0x409 -Iinclude

CXX = $(BASE)/bin/arm-wince-mingw32ce-g++
STRIP = $(BASE)/bin/arm-wince-mingw32ce-strip

DLLTOOL = $(BASE)/bin/arm-wince-mingw32ce-dlltool
DLLTOOLFLAGS =

define compile
@echo "  Compiling $1"
$(Q)$(CXX) $(CXXFLAGS) -c $1 -o $2
endef

$(OUT)%.o: %.cpp ; $(call compile,$<,$@)
$(OUT)%.o: %.S ; $(call compile,$<,$@)

$(OUT)%.o: %.rc
	@echo "  Building resource file from $<"
	$(Q)$(RC) $(RCFLAGS) -i $< -o $@

$(OUT)%.lib: src/wince/%.def
	@echo "  Building library $@"
	$(Q)$(DLLTOOL) $(DLLTOOLFLAGS) -d $< -l $@

$(OUT)%-debug:
	$(Q)echo 'const char *VERSION = "$(VERSION)";' > $(OUT)version.cpp
	$(call compile,$(OUT)version.cpp,$(OUT)version.o)
	@echo "  Checking for relocations"
	$(Q)tools/checkrelocs $(filter %.o,$^)
	@echo "  Linking $@ (Version \"$(VERSION)\")"
	$(Q)$(CXX) $(LDFLAGS) $(OUT)version.o $^ $(LIBS) -o $@

$(OUT)%.exe: $(OUT)%-debug
	@echo "  Stripping $^ to make $@"
	$(Q)$(STRIP) $^ -o $@

################ Haret exe rules

# List of machines supported - note order is important - it determines
# which machines are checked first.
MACHOBJS := machines.o \
  mach-universal.o \
  mach-autogen.o \
  arch-pxa27x.o arch-pxa.o arch-sa.o arch-omap.o arch-s3.o arch-920t.o

$(OUT)mach-autogen.o: src/mach/machlist.txt
	@echo "  Building machine list"
	$(Q)tools/buildmachs.py < $^ > $(OUT)mach-autogen.cpp
	$(call compile,$(OUT)mach-autogen.cpp,$@)

COREOBJS := $(MACHOBJS) haret-res.o \
  script.o memory.o video.o asmstuff.o lateload.o output.o cpu.o \
  linboot.o fbwrite.o font_mini_4x6.o winvectors.o

HARETOBJS := $(COREOBJS) haret.o gpio.o uart.o wincmds.o \
  watch.o irqchain.o irq.o pxatrace.o l1trace.o arminsns.o \
  network.o terminal.o com_port.o tlhcmds.o memcmds.o pxacmds.o aticmds.o

$(OUT)haret-debug: $(addprefix $(OUT),$(HARETOBJS)) src/haret.lds

####### Stripped down linux bootloading program.
LINLOADOBJS := $(COREOBJS) stubboot.o kernelfiles.o

KERNEL := zImage
INITRD := /dev/null
SCRIPT := docs/linload.txt

$(OUT)kernelfiles.o: src/wince/kernelfiles.S FORCE
	@echo "  Building $@"
	$(Q)$(CXX) -c -DLIN_KERNEL=\"$(KERNEL)\" -DLIN_INITRD=\"$(INITRD)\" -DLIN_SCRIPT=\"$(SCRIPT)\" -o $@ $<

$(OUT)linload-debug: $(addprefix $(OUT), $(LINLOADOBJS)) src/haret.lds

linload: $(OUT)linload.exe

####### Generic rules
clean:
	rm -rf $(OUT)

$(OUT):
	mkdir $@

-include $(OUT)*.d
