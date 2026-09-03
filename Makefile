#---------------------------------------------------------------------------------
# DATA DEFENSE 1.3.11 -- Switch homebrew loader
#
# Retargeted from the Killer Bean Unleashed tree (which itself came from the
# Fruit Ninja Classic + / PvZ Fusion / Zookeeper DX lineage).
#
#   this game: Unity 6000.3.9f1, IL2CPP, arm64, il2cpp metadata v39
#   Killer Bean: Unity 2021.3.31f1 -- three Unity majors older, which is why
#   ndk_choreographer.c exists (see the comment at the top of that file).
#
# Requires devkitA64 + devkitPro pkgs: switch-mesa switch-libdrm_nouveau
#                                      switch-sdl2 switch-zlib switch-libpng
#
# All .c files in source/ compile automatically, so imports_dd_extra.c and
# ndk_choreographer.c are picked up without editing any list here.
# nx_patch_datadefense.h and unity_entrypoints.h are headers included by main.c.
#---------------------------------------------------------------------------------
.SUFFIXES:
ifeq ($(strip $(DEVKITPRO)),)
$(error "Set DEVKITPRO in your environment. (export DEVKITPRO=/opt/devkitpro)")
endif
TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

TARGET    := datadefense_nx
APP_TITLE := Data Defense
APP_AUTHOR := ChanseyIsTheBest
APP_VERSION := 1.0.0
# No icon is shipped: the reference trees' icon.jpg files are other games'
# artwork and are not ours to redistribute. Drop your own 256x256 JPEG in as
# icon.jpg and uncomment APP_ICON, or build without one (libnx uses a default).
APP_ICON  := $(TOPDIR)/icon.jpg
export APP_TITLE APP_AUTHOR APP_VERSION
BUILD     := build
SOURCES   := source
INCLUDES  := source

ARCH    := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS  := -g -Wall -O2 -ffunction-sections $(ARCH) $(DEFINES) \
           $(INCLUDE) -D__SWITCH__
CFLAGS  += -DLOAD_ADDRESS=0xC0000000
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17
ASFLAGS := -g $(ARCH)
LDFLAGS  = -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map) \
           -Wl,--wrap,free -Wl,--wrap,malloc -Wl,--wrap,memalign -Wl,--wrap,calloc -Wl,--wrap,realloc -Wl,--wrap,memmove -Wl,--wrap,memcpy

# mesa GLES3 + EGL + nouveau, SDL2 for window/HID/audio, libpng for the optional
# cursor.png, zlib. -lpng must precede -lz: libpng calls into it.
LIBS := -lSDL2 -lGLESv2 -lEGL -lglapi -ldrm_nouveau -lpng -lz -lnx -lm

LIBDIRS := $(PORTLIBS) $(LIBNX)

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT  := $(CURDIR)/$(TARGET)
export TOPDIR  := $(CURDIR)
export VPATH   := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export LD := $(CXX)
export OFILES := $(addsuffix .o,$(SFILES)) $(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(PORTLIBS)/include/SDL2 -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: all clean check verify
all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
$(BUILD):
	@mkdir -p $@
# Run the static checks that a Linux host CAN do. Cheap; do it before every build.
check:
	@python3 tools/buildcheck.py source
	@python3 tools/scan_granularity.py --selftest
	@sh tools/syntaxcheck.sh || true
# Did the build pick up the source, and did that log come from this build?
#   make verify
#   make verify LOG=/path/to/sdcard/debug.log
verify:
	@python3 tools/verify_build.py $(if $(LOG),--log $(LOG))
clean:
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf
else
DEPENDS := $(OFILES:.o=.d)
# embed the icon + NACP (title/author/version) into the NRO asset section
NROFLAGS := --nacp=$(OUTPUT).nacp
ifneq ($(strip $(APP_ICON)),)
NROFLAGS += --icon=$(APP_ICON)
endif
all : $(OUTPUT).nro
$(OUTPUT).nro : $(OUTPUT).elf $(OUTPUT).nacp
$(OUTPUT).elf : $(OFILES)
-include $(DEPENDS)
endif
