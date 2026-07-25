.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment.")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

TARGET      := mangospot
BUILD       := build
SOURCES     := source source/ui source/player source/data source/render source/spotify
INCLUDES    := source
ROMFS       := $(TOPDIR)/romfs

APP_TITLE   := MangoSpot
APP_AUTHOR  := Manuel
APP_VERSION := 0.1.0

ARCH        := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS      := -g -Wall -O2 -ffunction-sections $(ARCH) $(DEFINES)
CFLAGS      += $(INCLUDE) -D__SWITCH__
# cspot needs RTTI/exceptions (LoginBlob, nlohmann::json, etc.) and C++20.
CXXFLAGS    := $(CFLAGS) -std=gnu++20
ASFLAGS     := -g $(ARCH)
# --allow-multiple-definition: bell vendors its own copy of libogg's framing.c
# (via tremor, for fixed-point Vorbis) which duplicates symbols already
# provided by the portlib -logg used for local mock playback. Both copies are
# ABI compatible; this just tells the linker to keep the first one it sees.
LDFLAGS     := -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map) -Wl,--allow-multiple-definition

# cspot (Spotify Connect client) is built separately via CMake against
# devkitpro/cmake/Switch.cmake - see SPOTIFY_INTEGRATION.md for the build
# command. We link the resulting static libs directly instead of migrating
# this whole project's build to CMake.
CSPOT_DIR      := $(TOPDIR)/external/cspot/cspot
CSPOT_BELL     := $(CSPOT_DIR)/bell
CSPOT_BUILD    := $(TOPDIR)/build-cspot-switch

CSPOT_INCLUDE  := -I$(CSPOT_DIR)/include \
                  -I$(CSPOT_BUILD) \
                  -I$(CSPOT_BELL)/main/audio-sinks/include \
                  -I$(CSPOT_BELL)/main/audio-dsp/include \
                  -I$(CSPOT_BELL)/main/utilities/include \
                  -I$(CSPOT_BELL)/main/io/include \
                  -I$(CSPOT_BELL)/main/platform \
                  -I$(CSPOT_BELL)/external/nanopb \
                  -I$(CSPOT_BELL)/external/nlohmann_json/include \
                  -I$(CSPOT_BELL)/external/fmt/include \
                  -I$(CSPOT_BELL)/external/tremor

CSPOT_LIBPATHS := -L$(CSPOT_BUILD) \
                  -L$(CSPOT_BUILD)/bell \
                  -L$(CSPOT_BUILD)/bell/external/opus \
                  -L$(CSPOT_BUILD)/bell/external/opencore-aacdec

CXXFLAGS       += $(CSPOT_INCLUDE)

LIBS := -lcspot -lbell -lopencore-aacdec -lopus -lmbedtls -lmbedx509 -lmbedcrypto \
        -lSDL2_mixer \
        -lmpg123 -lvorbisfile -lvorbis -lopusfile -lopus -lmodplug \
        -lSDL2_ttf -lSDL2_image -lSDL2 \
        -lharfbuzz -lfreetype \
        -lpng -ljpeg -lbz2 -lz -logg \
        -lEGL -lglapi -ldrm_nouveau \
        -lm -lnx

LIBDIRS     := $(PORTLIBS) $(LIBNX)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT   := $(CURDIR)/$(TARGET)
export TOPDIR   := $(CURDIR)
export VPATH    := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR  := $(CURDIR)/$(BUILD)

CFILES      := $(foreach dir,$(SOURCES),$(wildcard $(CURDIR)/$(dir)/*.c))
CPPFILES    := $(foreach dir,$(SOURCES),$(wildcard $(CURDIR)/$(dir)/*.cpp))
SFILES      := $(foreach dir,$(SOURCES),$(wildcard $(CURDIR)/$(dir)/*.s))

export LD       := $(CXX)
export OFILES   := $(notdir $(CFILES:.c=.o)) $(notdir $(CPPFILES:.cpp=.o)) $(notdir $(SFILES:.s=.o))
export INCLUDE  := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                   $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                   -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(CSPOT_LIBPATHS) $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export APP_ICON := $(TOPDIR)/icon.jpg

# elf2nro flags. WITHOUT --romfsdir the bundled romfs (fonts + mock songs) is
# NOT embedded in the .nro, so romfs:/ is empty at runtime. --nacp/--icon embed
# the title/author/version metadata and the home-menu icon.
export NROFLAGS := --icon=$(APP_ICON) --nacp=$(OUTPUT).nacp
ifneq ($(strip $(ROMFS)),)
export NROFLAGS += --romfsdir=$(ROMFS)
endif

.PHONY: $(BUILD) clean all

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf

else

DEPENDS := $(OFILES:.o=.d)

all: $(OUTPUT).nro

$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp
$(OUTPUT).elf: $(OFILES)

-include $(DEPENDS)

endif